/**
 * Copyright 2025 Beijing Volcano Engine Technology Co., Ltd. All Rights Reserved.
 * SPDX-license-identifier: BSD-3-Clause
 */

const Koa = require('koa');
const uuid = require('uuid');
const bodyParser = require('koa-bodyparser');
const cors = require('koa2-cors');
const { Signer } = require('@volcengine/openapi');
const fetch = require('node-fetch');
const { wrapper, assert, readFiles } = require('./util');
const TokenManager = require('./token');
const Privileges = require('./token').privileges;

// 加载场景配置（系统导出格式）
const SceneConfigs = readFiles('./scenes', '.json');

// 加载凭证配置
let Credentials = {};
try {
  Credentials = require('./credentials.json');
} catch (e) {
  console.error('警告: credentials.json 不存在，请创建该文件');
}

const { AccountConfig = {}, RTCConfig: GlobalRTCConfig = {}, Scenes: SceneMeta = {} } = Credentials;

// 运行时状态：存储每个场景的 RoomId、UserId 等
const RuntimeState = {};

const app = new Koa();

app.use(cors({
  origin: '*'
}));

app.use(bodyParser());

app.use(async ctx => {
  /**
   * @brief 代理 AIGC 的 OpenAPI 请求
   */
  await wrapper({
    ctx,
    apiName: 'proxy',
    containResponseMetadata: false,
    logic: async () => {
      const { Action, Version = '2025-06-01' } = ctx.query || {};
      assert(Action, 'Action 不能为空');
      assert(Version, 'Version 不能为空');

      const { SceneID } = ctx.request.body;

      assert(SceneID, 'SceneID 不能为空, SceneID 用于指定场景的 JSON');

      const sceneConfig = SceneConfigs[SceneID];
      assert(sceneConfig, `${SceneID} 不存在, 请先在 Server/scenes 下定义该场景的 JSON.`);

      assert(AccountConfig.accessKeyId, 'credentials.json 中 AccountConfig.accessKeyId 不能为空');
      assert(AccountConfig.secretKey, 'credentials.json 中 AccountConfig.secretKey 不能为空');
      assert(GlobalRTCConfig.AppId, 'credentials.json 中 RTCConfig.AppId 不能为空');

      const sceneMeta = SceneMeta[SceneID] || {};
      const state = RuntimeState[SceneID] || {};

      let body = {};
      switch(Action) {
        case 'StartVoiceChat':
          // 优先使用客户端传入的 RoomId 和 UserId
          const clientRoomId = ctx.request.body.RoomId;
          const clientUserId = ctx.request.body.UserId;
          
          const roomId = clientRoomId || state.RoomId;
          const targetUserId = clientUserId || state.UserId;
          
          assert(roomId, 'RoomId 不能为空');
          assert(targetUserId, 'UserId 不能为空');
          
          // 更新运行时状态
          RuntimeState[SceneID] = { ...state, RoomId: roomId, UserId: targetUserId };
          
          // 构建 VoiceChat 请求体（合并系统导出配置 + 运行时参数）
          body = {
            AppId: GlobalRTCConfig.AppId,
            RoomId: roomId,
            TaskId: sceneMeta.taskId || `task_${SceneID}_001`,
            AgentConfig: {
              ...sceneConfig.AgentConfig,
              TargetUserId: [targetUserId],
              UserId: sceneMeta.botUserId || `ai_bot_${SceneID}`,
              EnableConversationStateCallback: true
            },
            Config: sceneConfig.Config
          };
          console.log('StartVoiceChat 请求体:', JSON.stringify({
            AppId: body.AppId,
            RoomId: body.RoomId,
            TaskId: body.TaskId,
            TargetUserId: body.AgentConfig?.TargetUserId,
          }, null, 2));
          break;
        case 'StopVoiceChat':
          const stopRoomId = RuntimeState[SceneID]?.RoomId;
          // 如果 RoomId 为空，说明没有正在运行的任务，直接返回成功
          if (!stopRoomId) {
            return { Result: 'ok', message: 'No active task to stop' };
          }
          body = {
            AppId: GlobalRTCConfig.AppId,
            RoomId: stopRoomId,
            TaskId: sceneMeta.taskId || `task_${SceneID}_001`
          };
          break;
        default:
          break;
      }

      /** 参考 https://github.com/volcengine/volc-sdk-nodejs 可获取更多 火山 TOP 网关 SDK 的使用方式 */
      const openApiRequestData = {
        region: 'cn-north-1',
        method: 'POST',
        params: {
          Action,
          Version,
        },
        headers: {
          Host: 'rtc.volcengineapi.com',
          'Content-type': 'application/json',
        },
        body,
      };
      const signer = new Signer(openApiRequestData, "rtc");
      signer.addAuthorization(AccountConfig);
      
      /** 参考 https://www.volcengine.com/docs/6348/69828 可获取更多 OpenAPI 的信息 */
      const result = await fetch(`https://rtc.volcengineapi.com?Action=${Action}&Version=${Version}`, {
        method: 'POST',
        headers: openApiRequestData.headers,
        body: JSON.stringify(body),
      });
      return result.json();
    }
  });

  wrapper({
    ctx,
    apiName: 'getScenes',
    logic: () => {
      const { AppId, AppKey } = GlobalRTCConfig;
      assert(AppId, 'credentials.json 中 RTCConfig.AppId 不能为空');
      assert(AppKey, 'credentials.json 中 RTCConfig.AppKey 不能为空');
      
      const scenes = Object.keys(SceneConfigs).map((sceneId) => {
        const sceneConfig = SceneConfigs[sceneId];
        const sceneMeta = SceneMeta[sceneId] || {};
        
        // 生成 RoomId 和 UserId
        const RoomId = uuid.v4();
        const UserId = uuid.v4();
        
        // 存储到运行时状态
        RuntimeState[sceneId] = { RoomId, UserId };
        
        // 生成 Token
        const key = new TokenManager.AccessToken(AppId, AppKey, RoomId, UserId);
        key.addPrivilege(Privileges.PrivSubscribeStream, 0);
        key.addPrivilege(Privileges.PrivPublishStream, 0);
        key.expireTime(Math.floor(new Date() / 1000) + (24 * 3600));
        const Token = key.serialize();
        
        console.log(`场景 ${sceneId} 配置:`, { RoomId, UserId });
        
        // 构建场景信息
        const sceneInfo = {
          id: sceneId,
          name: sceneMeta.name || sceneId,
          icon: sceneMeta.icon || '',
          botName: sceneMeta.botUserId || `ai_bot_${sceneId}`,
          isInterruptMode: sceneConfig.Config?.InterruptMode === 0,
          isVision: sceneConfig.Config?.LLMConfig?.VisionConfig?.Enable,
          isScreenMode: sceneConfig.Config?.LLMConfig?.VisionConfig?.SnapshotConfig?.StreamType === 1,
          isAvatarScene: sceneConfig.Config?.AvatarConfig?.Enabled,
          avatarBgUrl: sceneConfig.Config?.AvatarConfig?.BackgroundUrl
        };
        
        return {
          scene: sceneInfo,
          rtc: { AppId, RoomId, UserId, Token },
        };
      });
      return {
        scenes,
      };
    }
  });
});

app.listen(3001, () => {
  console.log('AIGC Server is running at http://localhost:3001');
});

