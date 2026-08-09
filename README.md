# FT Remote

FT Remote 是一个使用 C++/Qt 6 编写的 FTBridge v1 桌面客户端，提供频谱、瀑布、CAT、RX/TX 音频和安全 PTT 控制。

## 构建

需要 Qt 6.8 或更高版本，并启用 `Core Gui Qml Quick QuickControls2 Network WebSockets Multimedia Test` 模块。密码记忆功能需要安装 QtKeychain；没有 QtKeychain 时客户端仍可运行，但不会把密码持久化到磁盘。CI 额外生成 Windows 安装包、macOS DMG、Linux AppImage 和压缩包。

```sh
cmake -S . -B build -DFTREMOTE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

若本机没有 QtKeychain，可以在有网络的构建环境中显式启用固定版本的 FetchContent：

```sh
cmake -S . -B build -DFTREMOTE_FETCH_QTKEYCHAIN=ON
```

## 运行安全边界

- 每次启动都显示服务器、用户名和密码登录窗；“记住密码”只使用操作系统钥匙串。
- 非本机连接必须使用 HTTPS/WSS。首次遇到未受信任证书时，用户必须核对并确认 SHA-256 指纹。
- PTT 使用独立的 press/release 消息；控制通道断开时本地立即停止 TX，服务端按协议安全回 RX。
- 目前媒体 attach 使用协议规定的 PCM S16LE 48 kHz/20 ms 基线；Opus 接入保留在可选编解码器扩展中。

## 工程结构

- `src/core`：协议、CAT、媒体解析、TLS/认证、凭据和音频。
- `src/app_controller.*`：QML 可调用的应用状态与动作门面。
- `qml`：登录窗、频谱/瀑布和右侧电台控制栏。
- `tests`：协议信封、CAT 校验和媒体帧单元测试。
