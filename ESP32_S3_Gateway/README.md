> **Trạng thái đồ án · 18/11/2026.** Nền R12 đã được thử cân bằng và dựng từ 60°. Điều khiển web có 8 hướng và log. Workspace đang ở **RT36-SYM / NET36**, đã build, kiểm thử trên máy và push Git. Mô hình tính pin kê thêm 10 mm. PCB một mặt đồng đã qua kiểm tra KiCad với 31 dây cầu. Xem [tổng quan và các việc còn mở](../Docs/Tong_Quan_Do_An.md).

> Các mô tả theo phiên bản cũ bên dưới là tài liệu đối chiếu. Dùng tổng quan phía trên để xác định cấu hình hiện hành.

# Bản hiện tại: RT36-SYM / NET36

Cần nạp STM32 `BAL-20261118-R12-8DIR-RT36-SYM` và ESP32
`TWIP-WEB-20261118-BAL-R12-8DIR-RT36-SYM-NET36`. Binary, hash và lệnh nạp ở
[`../Firmware/RT36_SYM_20261118/`](../Firmware/RT36_SYM_20261118/README.md). Mở
`http://192.168.4.1/ui/r12?v=RT36-SYM-NET36`.

NET36 chỉ đổi literal build ID so với NET35; hành vi gateway không đổi. Các thay đổi gateway đang
chạy, theo thứ tự:

- **NET34/35**: hạ TX Wi-Fi xuống **11 dBm**, **không ghi Flash khi xe đang cân bằng/chạy** (archive
  task chỉ ghi khi đã `settled`) — log RT34 có `reset=power_on`, hết brownout. Xem
  [RT33](../Docs/RT33_On_Dinh_Ket_Noi.md).
- **NET33**: ba nút **50 / 80 / MAX 90**, trần 0,90 m/s; log 122 byte/40 cột/50 Hz.
- **NET21**: SoftAP bật trước khi chuẩn bị log; trang tự đóng WebSocket nếu 3 s không nhận khung;
  route log/archive trả tên tệp trần; nút **SỬA VÙNG LOG FLASH** khi volume không mount.

## Lịch sử: RT22 SYM / NET23 (14/11/2026)

Cần nạp STM32 `BAL-20261114-R12-8DIR-RT22-SYM` và ESP32 `TWIP-WEB-20261114-BAL-R12-8DIR-RT22-SYM-NET23`. Mở `http://192.168.4.1/ui/r12?v=RT22-SYM-NET23`. Xem `../Docs/RT22_Doi_Xung_Va_Be_Huong.md`.

RT22 sửa phần điều khiển trên STM32 (ESP32 chỉ đổi literal build ID):

- Governor không còn **đóng băng** tham chiếu khi thấy thân nghiêng; nó giới hạn gia tốc còn 0,06 m/s² nên lệnh đang giữ vẫn được thực hiện, không khựng. Ngưỡng nghiêng 5° → **9°** (11° khi khởi hành) và tư thế trung tính học nhanh hơn, giữ qua STOP/ARM.
- Tiến và lùi tăng tốc như nhau và đạt **đúng** mốc 0,50 / 0,72 / 0,80 m/s (trước đây tiến bị ghìm 0,5–4,8 s, chỉ tới 0,61–0,79).
- Bẻ hướng gắt hơn: quay tại chỗ 2,4 rad/s, chéo 1,15 rad/s ở preset 80 (trước 0,50), gia tốc yaw 4,5 rad/s², trần gia tốc ngang 0,9 m/s².

Hiệu chuẩn gyro lúc cấp nguồn vẫn **500 mẫu ≈ 5,15 s** (đo 10,3 ms/mẫu ở ODR 100 Hz), nên từ cắm pin đến frame đầu tiên khoảng 5,3 s và tới lúc bấm BẬT ~5,5 s — trang hiện "Chưa nhận STM32" trong khoảng đó là do hiệu chuẩn, không phải Wi-Fi. Bản thử rút còn 200 mẫu (`QUICKSTART`, 14/11) đã được **khôi phục** về 500 (bản thử đó đã gỡ, xem git history).

> Bản thử thêm kênh chẩn đoán qua cáp (NET22, 14/11) đã được **gỡ bỏ** theo yêu cầu: chỉ cần dữ liệu qua log tải từ trang web.

NET21 (nền của RT22) sửa đường kết nối và log dự phòng, không sửa điều khiển STM32:

- SoftAP được bật **trước** khi chuẩn bị log Flash; chỉ quét toàn phân vùng 9,9 MB khi mount thất bại. Trước đây mỗi lần reset mất hàng giây trước khi có Wi-Fi.
- Trang tự đóng WebSocket nếu **3 s** không nhận khung nào từ gateway (kênh nửa-mở sau khi Wi-Fi điện thoại rớt), rồi thử lại sau 0,5 → 1 → 2 → 4 s. Gateway phát `{"hb":1}` 1 Hz và bật ping 20 s. Nhờ vậy "mất kênh lệnh" xuất hiện đúng lúc, không phải chờ TCP timeout.
- Khung log Flash liệt kê theo **tên tệp trần** (trước đây kèm `/twip-net1/` nên nút tải sẽ 404 dù volume mount được), và việc quét chỉ đọc header khối nên nhanh hơn nhiều.
- Nếu volume không mount được (`archive_error` khác `ready`), trang hiện nút **SỬA VÙNG LOG FLASH** (`POST /archive-format`, cần xác nhận ERASE, chỉ khi motor dừng). Đây là đường phục hồi duy nhất ngoài việc nạp lại/Erase All Flash — **mất mọi bản log còn trên Flash**.
- Trang phân biệt "ESP32 khởi động lại" với "STM32 không gửi": `/balance-status` trả `boot_id`, `uptime_ms`, `reset_reason`; sau một lần reset, trang báo `ESP32 ĐÃ KHỞI ĐỘNG LẠI (...)` trong 12 s kèm số mẫu log Flash còn lại.

**Brownout vẫn là lỗi nguồn.** NET21 không ngăn được reset; nó chỉ làm reset ngắn hơn và không mất log. Phải đo 5 V/3V3 tại ESP32 khi motor đảo chiều và thêm tụ bulk 470–1 000 µF.

> **Lịch sử RT4/NET1:** thông tin ở các mục mang tên RT4 phía dưới mô tả nền cũ. Dùng URL và hướng dẫn đầu tài liệu.

Đo lại gyro khóa các thao tác bật/thử bánh tới khi có dữ liệu xác nhận hoàn tất. STM32 hủy lệnh motor đang chờ và yêu cầu kiểm tra đứng yên lại đủ 200 ms; hiệu chuẩn không tự bật motor.

# ESP32-S3 Gateway — R12 RT4 NET1

NET1 bổ sung chẩn đoán reset và log Flash dự phòng, không đổi điều khiển STM32. Web `http://192.168.4.1/ui/r12?v=rt4-net1`. SSID vẫn `TWIP_RACER_S3`, AP kênh6/HT20, tắt power-save và đặt TX17dBm; không phát lại luồng TEL thừa cho dashboard R12. Lệnh giữ/nhả gửi ngay qua WebSocket, làm mới40ms; cập nhật giao diện200ms, bảng chi tiết500ms. Mất kết nối không tự chạy lại và không xóa log RAM.

Mục **Kết nối và log dự phòng** hiển thị số phiên khởi động, `esp_reset_reason`, bộ nhớ thấp nhất, số lần rời Wi-Fi và khoảng ngắt vòng web. Nút **TẢI CHẨN ĐOÁN KẾT NỐI** xuất JSON. `brownout` là ESP32 báo sụt áp; `power_on` cũng có thể do mất nguồn hoàn toàn; chỉ nhìn biểu tượng5G không đủ xác định nguyên nhân.

Log dự phòng dùng phân vùng FAT có sẵn của cấu hình16MB; không đổi phân vùng. Mount không phá dữ liệu: chỉ tự khởi tạo khi toàn bộ phân vùng còn0xFF. Nếu phân vùng có dữ liệu mà không mount được, giữ nguyên và báo lỗi. Tác vụ riêng chép tối đa50frame mỗi đợt, kiểm CRC và flush; sau reset chỉ xuất các đợt hợp lệ. Không giữ captureMutex khi ghi Flash hoặc gửi mạng. Tối đa4file, không tự xóa/ghi đè. Đầy thì báo rõ và dừng ghi dự phòng; log RAM và điều khiển vẫn hoạt động.

Khi dừng và chờ2s, bấm **TẢI LOG FLASH** ở file tương ứng. Sau khi đã lưu, có thể xóa bản cũ. Để xóa bản của phiên RAM hiện tại, tải trước rồi bấm xóa log RAM; điều này không xóa bản Flash. Ghi Flash là dự phòng tốt nhất có thể, không phải nguồn dự phòng: mất nguồn giữa đợt có thể mất phần cuối, hoặc làm hỏng FAT. Không chọn **Erase All Flash** khi muốn giữ log qua lần nạp firmware.

Trên iPhone, thử bật chế độ máy bay rồi bật lại Wi-Fi để loại việc tự chuyển sang mạng di động; giữ trang điều khiển phía trước. Mạng nội bộ không có Internet là bình thường. Nếu AP biến mất/xuất hiện lại và log RAM về0, kiểm tra lý do reset; đo5V tại chính chân cấp nguồn ESP32 khi motor khởi động/đảo chiều, không chỉ đo đầu ra XL4015 lúc không tải. Kiểm tra3V3, dây5V/GND và đường hồi dòng motor. Không tắt brownout/watchdog để che lỗi nguồn. Ăng-ten cần thoáng, tránh kim loại/dây động lực sát phần ăng-ten.

Mở `ESP32_S3_Gateway.ino` trong Arduino IDE. Cấu hình đã biên dịch: ESP32 core 3.3.11, thư viện WebSockets 2.7.2 của Markus Sattler.

- Board: ESP32S3 Dev Module (N16R8).
- USB CDC On Boot: Enabled.
- Flash Size: 16 MB.
- Partition Scheme: `app3M_fat9M_16MB` (3 MB APP).
- PSRAM: OPI PSRAM, bắt buộc để lưu log.

UART 115200 8N1: GPIO18 RX nối PB6 USART1 TX; GPIO17 TX nối PB7 USART1 RX; chung GND. Nạp STM32 cùng phiên bản giao thức.

Wi-Fi/AP được cấu hình trong sketch. Mở http://192.168.4.1/ui/r12?v=rt4-net1 . Web hiển thị trạng thái, lỗi, góc, tốc độ, PWM, encoder và tải log. Điều khiển giữ một trong8nút; giữ thì thực hiện liên tục, thả thì bỏ mục tiêu cũ và hãm về cân bằng tại điểm mới. Tiến/lùi có quãng đường hãm; quay đặt về0 ngay khi nhận neutral. Không xếp hàng lệnh giữ khi mạng chậm. Mặc định 50 cm/s, tùy chọn 72 cm/s đang cần thử thực tế. Không có giới hạn tổng quãng đường khi điều khiển liên tục; các giới hạn phục hồi và mất liên lạc vẫn được giữ.

Khi đổi tốc độ hoặc sau hủy mục tiêu do kẹt/phục hồi, cần nhả rồi giữ nút lại. Va chạm nghiêm trọng dừng motor, cần dựng xe và bật cân bằng lại. Khi bộ nhớ log đầy, tải log rồi xóa khi xe đã dừng trước lượt tiếp theo.
