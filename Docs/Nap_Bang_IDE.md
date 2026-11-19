# Nạp firmware bằng IDE (Build + Run / Upload)

Sáu bản firmware đều có **source để mở bằng IDE** và nạp bằng nút quen thuộc:

| Bản | Source STM32 (mở bằng STM32CubeIDE) | Sketch ESP32 (mở bằng Arduino IDE) |
|---|---|---|
| **RT40-HOLD / NET40** (hiện hành) | `D:\DA_HTN\Xe_Can_Bang` | `D:\DA_HTN\ESP32_S3_Gateway\ESP32_S3_Gateway.ino` |
| **RT39-CORNER / NET39** | `D:\DA_HTN\.build\ide_src\RT39\Xe_Can_Bang` | `D:\DA_HTN\.build\ide_src\RT39\ESP32_S3_Gateway\ESP32_S3_Gateway.ino` |
| **RT38-CONT / NET38** | `D:\DA_HTN\.build\ide_src\RT38\Xe_Can_Bang` | `D:\DA_HTN\.build\ide_src\RT38\ESP32_S3_Gateway\ESP32_S3_Gateway.ino` |
| **RT37-SMOOTH / NET37** | `D:\DA_HTN\.build\ide_src\RT37\Xe_Can_Bang` | `D:\DA_HTN\.build\ide_src\RT37\ESP32_S3_Gateway\ESP32_S3_Gateway.ino` |
| **RT34-CORNER / NET35** | `D:\DA_HTN\.build\ide_src\RT34\Xe_Can_Bang` | `D:\DA_HTN\.build\ide_src\RT34\ESP32_S3_Gateway\ESP32_S3_Gateway.ino` |
| **RT29-NOCUT / NET30** | `D:\DA_HTN\.build\ide_src\RT29\Xe_Can_Bang` | `D:\DA_HTN\.build\ide_src\RT29\ESP32_S3_Gateway\ESP32_S3_Gateway.ino` |

Hai thư mục `.build\ide_src\...` là **bản source đúng tại lúc phát hành**, tạo từ git. Nếu chúng chưa có
(máy mới, hoặc đã dọn `.build`), chạy một lần:

```powershell
cd D:\DA_HTN
powershell -NoProfile -ExecutionPolicy Bypass -File tools\ide_sources.ps1
```

Script in ra build ID của từng bản để kiểm. Không cần internet, không đổi repo.

> **Vì sao phải là source đúng bản?** STM32CubeIDE/Arduino IDE luôn **build lại từ source** rồi mới nạp.
> Build RT34 từ snapshot đã được kiểm: `.hex` và `.bin` ra **trùng SHA256** với bản đóng gói
> (`7ba6e0d1…` / `5c5237e6…`); RT29 trùng (`4cf9b2e9…` / `16effa54…`); RT37 trùng (`23257995…`);
> **RT38 trùng (`d075a0e2…`), RT39 trùng (`fc9d2f10…` / `89ba42e5…`) và RT40 trùng (`d4a60fdf…`)**. Riêng ESP32, bản build lại
> khác hash vì firmware nhúng dấu thời gian build — **cùng build ID và cùng hành vi**, chỉ khác hash.

## 1. Nạp STM32 bằng STM32CubeIDE

Áp dụng cho cả sáu bản, chỉ khác workspace.

**Sáu workspace đã được tạo sẵn trên máy này, project `Xe_Can_Bang` đã import và build sẵn** — mở là
thấy, không phải Import lại:

| Bản | Workspace mở bằng CubeIDE |
|---|---|
| RT40 | `D:\CubeIDE_ws` (workspace đang dùng) |
| RT39 | `D:\CubeIDE_ws_rt39` |
| RT38 | `D:\CubeIDE_ws_rt38` |
| RT37 | `D:\CubeIDE_ws_rt37` |
| RT34 | `D:\CubeIDE_ws_rt34` |
| RT29 | `D:\CubeIDE_ws_rt29` |

> Workspace chỉ là **thư mục cấu hình của IDE** (chứa `.metadata`), không chứa source. Source vẫn nằm
> ở `D:\DA_HTN\.build\ide_src\RT39\Xe_Can_Bang` (hoặc RT38/RT37/RT34/RT29); riêng RT40 dùng trực tiếp
> `D:\DA_HTN\Xe_Can_Bang`. Nếu thư mục này bị dọn, chạy `tools\ide_sources.ps1` rồi trong CubeIDE bấm
> **F5** (Refresh) ở project.

1. **Cắm ST-LINK** vào STM32 và máy tính.
2. Mở STM32CubeIDE → **File → Switch Workspace → Other…** → dán đường dẫn workspace ở bảng trên →
   **Launch**. (Nếu nhập một đường dẫn chưa tồn tại, CubeIDE **tự tạo** thư mục đó — không cần tạo tay
   trong Explorer.)
3. Ở **Project Explorer** đã có project `Xe_Can_Bang`. Chọn nó → bấm **Build** (biểu tượng búa).
   Chờ `0 errors, 0 warnings`.
4. Bấm **Run** (tam giác xanh) — lần đầu CubeIDE hỏi cấu hình, chọn
   **STM32 Cortex-M C/C++ Application** → **Run**. CubeIDE nạp hex qua ST-LINK rồi chạy.
5. Rút ST-LINK, cấp nguồn xe và kiểm banner (mục 3).

Nếu mở workspace mà **không thấy project** (ví dụ đã dọn `.build`): **File → Import → General →
Existing Projects into Workspace** → *Select root directory* trỏ tới thư mục `Xe_Can_Bang` của bản đó
(bảng ở đầu tài liệu) → **Finish**.

Nếu nút Run báo không tìm thấy ST-LINK: kiểm driver ST-LINK, thử cắm lại, hoặc
**Run → Run Configurations → Debugger → ST-LINK S/N → Scan**.

## 2. Nạp ESP32 bằng Arduino IDE

1. **Cắm cáp USB dữ liệu** vào ESP32 (cổng USB của board S3).
2. Mở Arduino IDE → **File → Open…** → chọn đúng tệp `.ino` của bản (bảng trên).
3. **Tools** đặt đúng (giống nhau cho cả sáu bản):

| Mục | Giá trị |
|---|---|
| Board | **ESP32S3 Dev Module** |
| USB CDC On Boot | **Enabled** |
| Flash Size | **16MB (128Mb)** |
| Partition Scheme | **16M Flash (3MB APP/9.9MB FATFS)** |
| PSRAM | **OPI PSRAM** |
| Upload Speed | 921600 |
| Port | COM của ESP32 (xem Device Manager) |

4. Bấm **Upload** (mũi tên →). Chờ `Done uploading`.
5. Rút cáp, cấp nguồn, kiểm banner.

> Arduino IDE sẽ **build lại** từ source nên hash `.bin` khác bản đóng gói (dấu thời gian build),
> nhưng build ID và hành vi đúng bản. Không dùng hash để so ở đường IDE.

## 3. Kiểm sau khi nạp

Mở Wi-Fi `TWIP_RACER_S3` / `12345678` → `http://192.168.4.1/ui/r12`:

| Bản | Banner | Nút tốc độ |
|---|---|---|
| RT40 | `RT40-HOLD-NET40` | 50 / 80 / MAX 90 |
| RT39 | `RT39-CORNER-NET39` | 50 / 80 / MAX 90 |
| RT38 | `RT38-CONT-NET38` | 50 / 80 / MAX 90 |
| RT37 | `RT37-SMOOTH-NET37` | 50 / 80 / MAX 90 |
| RT34 | `RT34-CORNER-NET35` | 50 / 80 / MAX 90 |
| RT29 | `RT29-NOCUT-NET30` | 50 / 72 / 80 |

Nếu trang báo “Chờ thông tin phiên bản STM32…” hoặc nút BẬT CÂN BẰNG bị khoá: **hai board lệch bản**
— nạp lại cả hai từ cùng một bản (đúng workspace/sketch ở trên).

Dòng `$BINFO` trong log tải về cũng phải chứa đúng build ID.

## 4. Bảng thao tác nhanh

| Muốn nạp | STM32CubeIDE | Arduino IDE |
|---|---|---|
| RT40 | workspace `D:\CubeIDE_ws` → **Build → Run** | mở `D:\DA_HTN\ESP32_S3_Gateway\ESP32_S3_Gateway.ino`, **Upload** |
| RT39 | workspace `D:\CubeIDE_ws_rt39` → **Build → Run** | mở `.build\ide_src\RT39\ESP32_S3_Gateway\ESP32_S3_Gateway.ino`, **Upload** |
| RT38 | workspace `D:\CubeIDE_ws_rt38` → **Build → Run** | mở `.build\ide_src\RT38\ESP32_S3_Gateway\ESP32_S3_Gateway.ino`, **Upload** |
| RT37 | workspace `D:\CubeIDE_ws_rt37` → **Build → Run** | mở `.build\ide_src\RT37\ESP32_S3_Gateway\ESP32_S3_Gateway.ino`, **Upload** |
| RT34 | workspace `D:\CubeIDE_ws_rt34` → **Build → Run** | mở `.build\ide_src\RT34\ESP32_S3_Gateway\ESP32_S3_Gateway.ino`, **Upload** |
| RT29 | workspace `D:\CubeIDE_ws_rt29` → **Build → Run** | mở `.build\ide_src\RT29\ESP32_S3_Gateway\ESP32_S3_Gateway.ino`, **Upload** |

## 5. Lưu ý

- Không nạp khi xe đang cân bằng/đang chạy; bánh kê cao.
- Luôn nạp **cả hai board** cùng bản; đừng trộn RT39 với NET40.
- Mức 90 chỉ dùng khi pin đo **≥ 12,0 V**; RT29 chỉ tới 80 và không có bù bánh trái/trần cua.
- Cách nạp bằng script (`Firmware\nap_chon_ban.bat`) vẫn dùng được nếu không muốn mở IDE.
