# RT11 MAX95 — tăng mạnh tốc độ và phản hồi theo yêu cầu thử thực tế

Người dùng chủ động yêu cầu tăng mạnh để thử/lấy log, ưu tiên dữ liệu xe thật thay vì lấy mô phỏng làm chuẩn duyệt. Bản này là ứng viên thử **50/72/MAX95cm/s**, chưa được xác nhận trên xe. Khối lượng hồ sơ888g, nguồn khai báo12V; không có ADC điện áp/dòng.

## Hai log29/30 nói gì

- Đúng nềnRT10-MAX80/NET7. MứcMAX: file29 đỉnh tiến0,86846m/s, đỉnh lùi−0,89189m/s; file30 tương ứng0,85885/−0,90358m/s. Đó là đỉnh encoder lúc điều khiển, không phải tốc độ ổn định liên tục.
- PWM lớn nhất khiMAX:2499/2499 trong29 và2424/2499 trong30. Không kết luận còn dư lực trong mọi tình huống. File29 có fault8192(va chạm mạnh) và16(vượt trần vận tốc); file30 không có fault trong phần đã lưu.
- Cả hai đã đầy9000mẫu. Số mẫu không lưu thêm:6926/357. CRC0, sequence gaps0, STM32 resets0. Vì buffer đóng băng khi đầy, không thể dùng phần cuối bị thiếu để kết luận thời điểmSTOP hay lỗi sau đó.
- Khi đang nhận hướng, tuổi lệnh ởSTM32 (t_ms−drive_rx_ms): trung vị23ms, P95=44ms, lớn nhất93/98ms. Đây là tuổi lệnh gần nhất tại mẫu ghi, **không phải** độ trễ từ lúc ngón tay chạm tới motor. Thời gian vòng điều khiển ghi lớn nhất638µs, trong chu kỳ5ms.
- Mã cũ tăng tốc0,30m/s² dưới0,5m/s,0,15 phía trên; quay nhanh2,2rad/s với dốc1,5rad/s² cần1,47giây đạt đặt. Đảo chiều còn phải hãm chuyển động đang có. Đây là nguồn cảm giác chậm có căn cứ, không quy toàn bộ choWi-Fi.

## Thay đổi

1. MAX80→**MAX95cm/s**, tăng18,75% điểm đặt;50/72 giữ nguyên tốc độ đặt. Với bánh65mm,95cm/s tương đương279rpm nếu không trượt. Đây là mục tiêu lệnh; motor có tải có thể không đạt và có thể bão hòa/mất cân bằng. Không ép PWM100% cố định, vẫn dùng R12 phân bổ lực.
2. Tăng dốc tiến/lùi cho các mức:0,60m/s² dưới0,5m/s,0,30 phía trên. Nhả hãm0,90; đảo chiều hãm1,0m/s². Theo tham chiếu lý tưởng, lên95 mất khoảng2,34giây; từ+95 tới điểm đặt0 khi đảo chiều khoảng0,95giây. Không hứa vận tốc xe thật đổi tức thời.
3. Dốc quay1,5→3,0rad/s²; dốc hãm/đảo quay6→8rad/s². Mức quay nhanh2,2rad/s giữ nguyên, đạt tham chiếu trong khoảng0,74giây. Xóa tích phân cũ khi hướng quay nhận mới đảo dấu, tránh lực quay còn mang nhớ của hướng cũ. Luật/gains R12/observer/hiệu chuẩn motor không đổi.
4. Governor từng bánh khiMAX dùng nền1,05m/s thay0,80, để có quyền quẹo ở tốc độ cao; giới hạn gia tốc ngang0,4m/s² và governor góc/tracking vẫn còn. Mức50/72 giữ nền governor0,80m/s.
5. MAX predictor overspeed chỉ xét trên1,05m/s; trần cứngMAX trung bình1,12m/s, mỗi bánh1,20m/s. Envelope giữ qua hãm/đổi mức nhưRT10. Bảo vệ góc, lỗiIMU, timing, va chạm, tracking, timeout và lease350ms vẫn còn. Không áp envelopeMAX vào lúc đang dựng xe/chu kỳARM đầu.
6. Đổi giữa hai lần nhấn primary pointer chuyển trực tiếp sang hướng mới, không chèn gói trung tính rồi mới gửi hướng mới. Pointer cũ nhả không được hủy pointer mới; ngón phụ không phải primary vẫn bị bỏ qua.
7. Gia hạnMOVE40→20ms; sự kiện đổi hướng vẫn gửi ngay. Pollhiển thị200→100ms. Không thay WebSocket bằng đường chờHTTP để điều khiển; không chồng thêm tác vụ đọc cùngsocket. Thư việnWebSockets đang dùng đã bậtTCP_NODELAY sẵn, không nhận đó là thay đổi mới.
8. Buffer log9000→30000mẫu, tương đương10phút hoạt động ở50Hz; giữ đóng băng khi đầy để tránh ghi đè bằng chứng. File mới vẫn40cột, frame122byte. Mỗi lượt nên dừng, tải log rồi xóa log trước lượt kế tiếp.

## Kiểm chứng và dùng bản thử

Kiểm thử native parser/FSM, session, HTTP, web đều qua; header luật/paramsR12 và UART hai MCU còn khớp. `test_rt11_response.cjs` kiểm đúng tham chiếu95, dốc đảo tuyến tính/quay, mấtlease; kiểmweb thêm việc đổi pointer chỉ phát một gói hướng mới và không để pointer cũ nhả nó. Có hồi quy supervisorREC. Đây là kiểm phần mềm/tham chiếu, chưa chứng minh độ ổn định vật lý của95 hoặc các dốc mới. Lượt này không lấy mô phỏng làm bằng chứng rằng xe chắc chắn giữ được95.

STM32 `BAL-20261031-R12-8DIR-RT11-MAX95`, ESP32 `TWIP-WEB-20261031-BAL-R12-8DIR-RT11-MAX95-NET8`. Bản nạp riêng `.build/balance_release_r12_rt11_max95_20261031`, không ghi đèRT10 dự phòng. Cần nạp cả hai; mở `http://192.168.4.1/ui/r12?v=rt11-max95-net8`.

Gửi log mới của chạy thẳng và đổi hướng kèm mô tả thứ tự thao tác. Log không có timestamp ngón tay nên chỉ tách được tuổi lệnhSTM32, tham chiếu, tốc độ/góc và bảo vệ; chưa đo được độ trễ toàn tuyến một chiều. Wi-Fi không bảo đảm thời gian đáp ứng cứng. Khi đang phục hồi cân bằng, điều khiển hướng vẫn phải chờ điều kiện cho phép như nền hiện hành.

Các log này vẫn báo `archive_error=1/archive_saved=0`; tăngRAMlog không sửa lỗi lưuFFat đó hoặc lỗiBMI160 sau ngã mạnh đã báo trước.

## Bản build cuối

Cả hai MCU build thành công. STM32:text73836/data492/bss7444byte; ESP32:1074147byte flash(34%),54004byte RAMtĩnh(16%), SDK3.3.11, flash16MB/PSRAMopi/CDC. Đã kiểm trực tiếpBIN có đúng buildID, MAX95, chu kỳgia hạn20ms/poll100ms và không còn chuỗi nhận diệnARM trung gianRT10-MAX95. SHA-256 cùng dấu nguồnLF trong `manifest.json` ở thư mục bản nạp.

LượtESP đầu bị lỗi công cụ sao chép ELF vào thư mục mặc định `ESP32_S3_Gateway/build` do file đang được ánh xạ. Lượt cuối chỉ bỏ hook sao chép đó cho lần gọiCLI, xuất thành công vào `.build/balance_release_r12_rt11_max95_20261031/esp32`. Dùng thư mục bản nạp riêng đã kiểm, không lấy nhầm file ở thư mục `build` mặc định cũ. Không sửa thiết lậpSDK toàn máy, không nạp hoặc push trong lượt này.
