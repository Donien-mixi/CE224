# Nạp firmware bằng IDE (Build + Run / Upload) — bản RT45-CLEAN / NET45

Chỉ còn **một bản** firmware trong repo. Hai đường nạp bằng IDE:

| Board | Mở ở đâu | Nút |
|---|---|---|
| **STM32F411** (`BAL-20261121-R12-8DIR-RT45-CLEAN`) | STM32CubeIDE → workspace **`D:\CubeIDE_ws`** | **Build** (búa) → **Run** (tam giác xanh) |
| **ESP32-S3** (`TWIP-WEB-20261121-BAL-R12-8DIR-RT45-CLEAN-NET45`) | Arduino IDE → **`D:\DA_HTN\ESP32_S3_Gateway\ESP32_S3_Gateway.ino`** | **Upload** (mũi tên →) |

Source STM32 nằm ở **`D:\DA_HTN\Xe_Can_Bang`** — workspace `D:\CubeIDE_ws` đã import sẵn project
`Xe_Can_Bang` và build sẵn, nên mở là bấm Build/Run được ngay (project trỏ vào thư mục nguồn, nên luôn
build đúng code hiện tại).

## 1. Nạp STM32 bằng STM32CubeIDE

1. **Cắm ST-LINK** vào STM32 và máy tính.
2. Mở STM32CubeIDE → **File → Switch Workspace → Other…** → dán `D:\CubeIDE_ws` → **Launch**.
   (Nếu nhập một đường dẫn chưa tồn tại, CubeIDE **tự tạo** thư mục đó — không cần tạo tay.)
3. Ở **Project Explorer** đã có project `Xe_Can_Bang`. Chọn nó → **Build**. Chờ `0 errors, 0 warnings`.
4. Bấm **Run** → lần đầu chọn **STM32 Cortex-M C/C++ Application** → **Run**. CubeIDE nạp hex qua
   ST-LINK rồi chạy.
5. Rút ST-LINK, cấp nguồn xe và kiểm banner (mục 3).

Nếu mở workspace mà **không thấy project**: **File → Import → General → Existing Projects into
Workspace** → *Select root directory* = `D:\DA_HTN\Xe_Can_Bang` → **Finish**.

Nếu **Run** báo không tìm thấy ST-LINK: kiểm driver ST-LINK, cắm lại, hoặc
**Run → Run Configurations → Debugger → ST-LINK S/N → Scan**.

## 2. Nạp ESP32 bằng Arduino IDE

1. **Cắm cáp USB dữ liệu** vào ESP32 (cổng USB của board S3).
2. Arduino IDE → **File → Open…** → `D:\DA_HTN\ESP32_S3_Gateway\ESP32_S3_Gateway.ino`.
3. **Tools** đặt đúng:

| Mục | Giá trị |
|---|---|
| Board | **ESP32S3 Dev Module** |
| USB CDC On Boot | **Enabled** |
| Flash Size | **16MB (128Mb)** |
| Partition Scheme | **16M Flash (3MB APP/9.9MB FATFS)** |
| PSRAM | **OPI PSRAM** |
| Upload Speed | 921600 |
| Port | COM của ESP32 (xem Device Manager) |

4. Bấm **Upload**. Chờ `Done uploading`.
5. Rút cáp, cấp nguồn, kiểm banner.

> Arduino IDE **build lại** từ source nên hash `.bin` khác bản đóng gói (dấu thời gian build), nhưng
> build ID và hành vi đúng bản. Không dùng hash để so ở đường IDE.

## 3. Kiểm sau khi nạp

Mở Wi-Fi `TWIP_RACER_S3` / `12345678` → `http://192.168.4.1/ui/r12`:

| Mục | Giá trị đúng |
|---|---|
| Banner | **`RT45-CLEAN-NET45`** |
| Nút tốc độ | **50 / 80 / MAX 90** |
| Serial ESP32 | `[UART] … @ 921600 bps` và `[NET41] … -> AP chN` |
| Log tải về | `$BINFO,BAL-20261121-R12-8DIR-RT45-CLEAN,…`, có `# RADIO ch=… rssi=… phy=…`, `crc_errors=0` |

Nếu trang báo “Chờ thông tin phiên bản STM32…” hoặc nút BẬT CÂN BẰNG bị khoá: **hai board lệch bản**
— nạp lại cả hai từ cùng bản này.

## 4. Lưu ý

- Không nạp khi xe đang cân bằng/đang chạy; bánh kê cao.
- **Luôn nạp cả hai board**: UART 921600 ở cả hai đầu, lệch một board là mất liên lạc.
- Mức MAX 90 chỉ đạt đúng 0,90 m/s khi pin **≥ 12,0 V**.
- Cách nạp bằng script (không cần mở IDE): `Firmware\RT45_CLEAN_20261121\nap_rt45.bat` (quay lui: `nap_rt44.bat`).
- Bản firmware cũ (RT29 → RT43) đã xoá khỏi cây làm việc; vẫn lấy lại được bằng `git history`.
