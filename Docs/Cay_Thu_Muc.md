# Cây thư mục — cái gì là gì, cái nào xoá được

Một trang để bạn không phải đoán. Quy tắc: **file do bạn/người viết ⇒ sửa và commit**; **file máy sinh
ra ⇒ xoá được, build lại là có**.

## 1. Phần bạn mở bằng IDE

| Thư mục | Là gì | Mở bằng |
|---|---|---|
| `Xe_Can_Bang/` | **Project STM32CubeIDE** — firmware cân bằng. `Core/Src/*.c` là code, `Core/Inc/*.h` là khai báo + thông số. `Debug/` là **thư mục build** (máy sinh) | STM32CubeIDE, workspace `D:\DA_HTN`, project `Xe_Can_Bang` |
| `ESP32_S3_Gateway/` | **Sketch Arduino** — Wi-Fi, web, ghi log. `ESP32_S3_Gateway.ino` là file chính; `dashboard_html.h` là trang web nhúng; `balance_web.h`, `network_routes.h` là các đường HTTP | Arduino IDE, mở file `.ino` |

Trong `Xe_Can_Bang/Core`:

| Nhóm file | Nội dung | Bạn có thể sửa |
|---|---|---|
| `balance_profile.h` | build ID, luật R12 đóng băng, cổng ARM, capture, chốt an toàn cấp hệ | có (rất cẩn thận) |
| `tuning.h` | **toàn bộ thông số chỉnh được** (tốc độ, cua, REC1, governor, giám sát tốc độ, bù bánh trái) | **đây là file để chỉnh xe** |
| `balance_motion.h` + `Core/Src/balance_motion.c`, `balance_turn.h` + `Core/Src/balance_turn.c` | quỹ đạo tốc độ và cua (thân hàm đã tách ra `.c`) | có |
| `balance_recovery.h`, `balance_governor.h`, `balance_interlock.h`, `balance_trim.h` | lớp giám sát: chống đẩy, giữ lại lực, trần tốc độ, bù bánh trái | có |
| `cat_control.*` (nếu có), `motor.c`, `encoder.c`, `bmi160.c`, `filter.c` | luật lực R12, động cơ, encoder, IMU, lọc | **không** (đóng băng) |
| *(đã xoá ở RT46)* `pid.c/.h`, `balance_assist.h`, `wheel_measurement.h` | cascade PID và 2 header không còn dùng — không ảnh hưởng firmware | — |
| `robot_fsm.c` | vòng 200 Hz + máy trạng thái | có (theo thứ tự pipeline) |
| `main.c`, `tim.c`, `usart.c`, `i2c.c`, `gpio.c`, `dma.c` | khởi tạo phần cứng (CubeMX sinh) | hạn chế |
| `system_stm32f4xx.c`, `stm32f4xx_hal_conf.h`, `stm32f4xx_it.c` | CubeMX/HAL sinh ra | **không sửa** |

## 2. File máy sinh ra — xoá được, không mất gì

| Thư mục | Dung lượng | Xoá được? | Sinh lại bằng |
|---|---:|---|---|
| `.build/` | — (đã xoá) | **có** | chạy test / `node tools/run_governor_sim.cjs` |
| `Xe_Can_Bang/Debug/` | — (đã xoá) | **có** | bấm **Build** trong CubeIDE (tự sinh lại) |
| `Simulation/results/` | — (đã xoá) | **có** | `node tools/run_governor_sim.cjs --stress` |
| `.metadata/` | — (đã xoá) | **có** | CubeIDE tự tạo lại (workspace hiện tại là `D:\DA_HTN`) |
| `.dsh/`, `.vscode/` | nhỏ | không cần xoá | — |

## 3. Tài liệu, bản nạp, công cụ

| Thư mục | Là gì |
|---|---|
| `Docs/` | Báo cáo từng vòng (`RTxx_*.md`), hướng dẫn nạp, luật điều khiển, kế hoạch dọn code |
| `tools/` | 38 host test (38 tệp `test_*.cjs`), script chạy mô phỏng |
| `Simulation/` | Mô hình Scilab + harness native dùng chung header firmware (`Simulation/current/`) |
| `Hardware/` | Sơ đồ, PCB KiCad, footprint |
| `output/` | Log thô đã tải về + script phân tích theo từng vòng (`output/RTxx/`) |

## 4. Log thô của bạn (chưa nằm trong git — cẩn thận)

Bốn thư mục này chứa log `.txt` tải từ xe và **không được commit** (để repo nhẹ). Xoá là mất vĩnh viễn:

`12 bài test động cơ và encoder/`, `56 bài test/`, `bài test 2-7/`, `test dưới sàn/`, `test kê cao/`.

Nên giữ hoặc nén lại khi làm báo cáo; nếu muốn lưu trong repo thì đưa vào `output/RTxx/logs/` (đang
được gitignore) hoặc nén thành `.zip` rồi commit.

## 5. Ba việc nhanh

```powershell
# Build + nạp: mở STM32CubeIDE → Clean → Build → Run (IDE tự tạo Debug/)

# Chạy toàn bộ 38 test trên máy (không cần xe)
Get-ChildItem tools\test_*.cjs | ForEach-Object { node $_.FullName }

# Chạy mô phỏng 12 ca (mất ~1 phút, sinh Simulation/results)
$env:TWIP_SIM_LABEL='kt'; node tools\run_governor_sim.cjs --stress
```
