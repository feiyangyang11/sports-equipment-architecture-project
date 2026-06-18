# T6 测试套件

本目录用于执行《软件体系结构》课程项目的 T6：功能性测试和非功能性测试。

测试使用独立数据库 `sports_equipment_management_test`，不会修改默认演示数据库。

## 一键执行

在项目根目录运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\run-t6.ps1
```

测试运行器会依次：

1. 重新构建 C++ 后端。
2. 删除并重建专用测试数据库。
3. 生成仅供测试后端读取的数据库配置。
4. 在 `127.0.0.1:8000` 启动后端。
5. 执行功能接口测试。
6. 执行 200 请求性能测试。
7. 执行同一预约的并发借出一致性测试。
8. 使用 Edge 和 Chrome 无头模式检查登录页，并检查主要静态资源。
9. 重启后端，验证旧 Token 失效且已提交数据仍然存在。
10. 将原始结果写入 `tests/results/`。
11. 停止测试后端。

## 目录说明

```text
tests/
├─ functional/
│  └─ run-functional-tests.ps1
├─ nonfunctional/
│  ├─ performance_test.py
│  └─ concurrency_test.py
├─ results/                 运行后生成 JSON 和日志
├─ runtime/                 运行后生成测试配置和临时 SQL
├─ setup-test-environment.ps1
└─ run-t6.ps1
```

## 注意事项

- 需要 MySQL 8.0 正常运行。
- 默认从 `backend/config/database.conf` 读取本机数据库连接信息。
- 需要 `C:\msys64\ucrt64\bin` 中的运行库。
- 端口 `8000` 必须空闲。
- 测试数据库会在每次运行时重建，请勿在其中保存人工数据。
