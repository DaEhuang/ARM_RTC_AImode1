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

const Scenes = readFiles('./scenes', '.json');

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
      const { Action, Version = '2024-12-01' } = ctx.query || {};
      assert(Action, 'Action 不能为空');
      assert(Version, 'Version 不能为空');

      const { SceneID } = ctx.request.body;

      assert(SceneID, 'SceneID 不能为空, SceneID 用于指定场景的 JSON');

      const JSONData = Scenes[SceneID];
      assert(JSONData, `${SceneID} 不存在, 请先在 Server/scenes 下定义该场景的 JSON.`);

      const { VoiceChat = {}, AccountConfig = {} } = JSONData;
      assert(AccountConfig.accessKeyId, 'AccountConfig.accessKeyId 不能为空');
      assert(AccountConfig.secretKey, 'AccountConfig.secretKey 不能为空');

      let body = {};
      switch(Action) {
        case 'StartVoiceChat':
          // 使用 RTCConfig 中的 RoomId（由 getScenes 生成）
          const { RTCConfig: rtc = {} } = JSONData;
          if (rtc.RoomId) {
            VoiceChat.RoomId = rtc.RoomId;
          }
          if (rtc.UserId) {
            VoiceChat.AgentConfig.TargetUserId[0] = rtc.UserId;
          }
          body = VoiceChat;
          console.log('StartVoiceChat 请求体:', JSON.stringify({
            AppId: VoiceChat.AppId,
            RoomId: VoiceChat.RoomId,
            TaskId: VoiceChat.TaskId,
            TargetUserId: VoiceChat.AgentConfig?.TargetUserId,
          }, null, 2));
          break;
        case 'StopVoiceChat':
          const { AppId, RoomId, TaskId } = VoiceChat;
          // 如果 RoomId 为空，说明没有正在运行的任务，直接返回成功
          if (!RoomId) {
            return { Result: 'ok', message: 'No active task to stop' };
          }
          assert(AppId, 'VoiceChat.AppId 不能为空');
          assert(TaskId, 'VoiceChat.TaskId 不能为空');
          body = {
            AppId, RoomId, TaskId
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
      const scenes = Object.keys(Scenes).map((scene) => {
        const { SceneConfig, RTCConfig = {}, VoiceChat } = Scenes[scene];
        const { AppId, RoomId, UserId, AppKey, Token } = RTCConfig;
        assert(AppId, `${scene} 场景的 RTCConfig.AppId 不能为空`);
        
        // 只在 RoomId 或 UserId 为空时生成新的（首次调用）
        // Token 每次都重新生成以确保有效
        const needGenerateIds = !RoomId || !UserId;
        if (needGenerateIds) {
          RTCConfig.RoomId = VoiceChat.RoomId = uuid.v4();
          RTCConfig.UserId = VoiceChat.AgentConfig.TargetUserId[0] = uuid.v4();
        }
        
        // 每次都重新生成 Token（Token 有过期时间）
        assert(AppKey, `自动生成 Token 时, ${scene} 场景的 AppKey 不可为空`);
        const key = new TokenManager.AccessToken(AppId, AppKey, RTCConfig.RoomId, RTCConfig.UserId);
        key.addPrivilege(Privileges.PrivSubscribeStream, 0);
        key.addPrivilege(Privileges.PrivPublishStream, 0);
        key.expireTime(Math.floor(new Date() / 1000) + (24 * 3600));
        RTCConfig.Token = key.serialize();
        
        console.log(`场景 ${scene} 配置:`, {
          RoomId: RTCConfig.RoomId,
          UserId: RTCConfig.UserId,
          VoiceChatRoomId: VoiceChat.RoomId,
          TargetUserId: VoiceChat.AgentConfig.TargetUserId[0],
        });
        
        SceneConfig.id = scene;
        SceneConfig.botName = VoiceChat?.AgentConfig?.UserId;
        SceneConfig.isInterruptMode = VoiceChat?.Config?.InterruptMode === 0;
        SceneConfig.isVision = VoiceChat?.Config?.LLMConfig?.VisionConfig?.Enable;
        SceneConfig.isScreenMode = VoiceChat?.Config?.LLMConfig?.VisionConfig?.SnapshotConfig?.StreamType === 1;
        SceneConfig.isAvatarScene = VoiceChat?.Config?.AvatarConfig?.Enabled;
        SceneConfig.avatarBgUrl = VoiceChat?.Config?.AvatarConfig?.BackgroundUrl;
        delete RTCConfig.AppKey;
        return {
          scene: SceneConfig || {},
          rtc: RTCConfig,
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

