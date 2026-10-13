# Bàn giao phát triển — R10S1

## Trạng thái

Firmware STM32: `BAL-20261012-R10S1`; ESP32: `TWIP-WEB-20261012-BAL-R10S1`. Đã biên dịch hai board và kiểm tra phần mềm. R10 trước đó có lượt tự cân bằng khoảng165giây nhưng còn quay/trôi và các lượt thất bại. R10S1 thay quy trình bắt đầu; chưa được kiểm chứng tốt hơn trên xe. Không coi là đã hoàn thành nghiệm thu GĐ1.

Repo có nguồn firmware, HAL/CMSIS, dự án CubeIDE, cấu hình CubeMX, web và mô phỏng độc lập. Không cần dữ liệu IDE, bản build, log hay báo cáo thử nghiệm cũ để bắt đầu.

## Vị trí thay đổi

| Việc cần làm | Nơi đọc/sửa |
|---|---|
| PID/hiệu chuẩn, encoder PPR, deadband, ngưỡng | `Xe_Can_Bang/Core/Inc/balance_profile.h` |
| PID góc, vận tốc, vị trí | `Core/Src/pid.c` |
| Bộ lọc góc và mốc lắp đặt | `Core/Src/filter.c`, `Core/Inc/filter.h` |
| BMI160 và đo bias gyro | `Core/Src/bmi160.c` |
| Encoder, dấu, lọc tốc độ | `Core/Src/encoder.c`, `Core/Inc/encoder.h` |
| ARM/STOP, điều khiển200Hz và telemetry | `Core/Src/robot_fsm.c` |
| Bù chênh hai bánh | `Core/Inc/balance_assist.h` |
| Phản hồi bắt lại/kiểm tra trạng thái đầu | `Core/Inc/balance_capture.h` |
| Lệnh UART | `Core/Src/esp32_comm.c` |
| Web và build ESP32 | `ESP32_S3_Gateway/README.md` |
| Bài mô phỏng riêng | `Simulation/config/team_run.sce`, `Simulation/scripts/run_team.sce` |

Các đường `Core/...` ở bảng thuộc `Xe_Can_Bang/`. Nhóm được thay controller/phương pháp tuning theo hướng riêng; các macro/hệ số hiện tại là điểm xuất phát.

## Các quy ước đang dùng

- Phía trước là WAGO phân phối nguồn pin; phía sau là WAGO nguồn5V.
- PWM dương làm đỉnh cảhai bánh quay về phía trước.
- Pitch điều khiển dương khi nghiêng về trước; khác dấu góc raw accel cũ. GyroY giữ dấu trong vòng điều khiển.
- Encoder phải đảo dấu ở driver. PPR điều khiển898,225ticks/vòng bánh; không nhân quadrature thêm lần nữa.
- PID nền `[48,3.6,32,0.5,0.3]`, góc tính bằng độ, tốc độm/s, vị trím, PWM±2499. PWM0 dùng phanh điện.
- Bù hướng cap±8PWM; chưa xác nhận xử lý hết quay dưới tải. Không đổi PPR để bù cơ khí/motor.
- Tích phân vận tốc mở sau3giây với điều kiện góc/tốc độ, cap±2°. Capture gain tăng từ1 tới1,75 khi |pitch| đi từ3° tới6°.
- ARM thủ công qua web: góc±2°, dự báo0,15s, gyro mỗi trục<3°/s, bánh mỗi bên<0,02m/s, góc lọc/accel chênh≤0,75° trong300ms; chờ1,5s sau gyro. Web cho bấm trước/chờ20s.
- STOP: góc25°, quãng encoder30cm, tốc độ0,8m/s, heartbeat1s và các lỗi đọc/timing/số. Giới hạn quãng encoder không phải tọa độ ngoài đo bằng camera.

## Build từ clone sạch

CubeIDE2.1.1 đã dùng thành công. Import project `Xe_Can_Bang` vào workspace riêng, Build Debug; IDE tạo lại makefile, object và binary. Giữ `.project`, `.cproject`, `.ioc`, `.settings`, `Core`, `Drivers`, linker scripts. Không sao chép workspace `.metadata` của người khác. `.ioc` chưa mô tả hết mã ứng dụng tự viết; khi Generate Code cần xem diff để tránh mất phần ngoài USER CODE.

ESP32 core3.3.11/WebSockets2.7.2 đã dùng thành công; cấu hình N16R8/OPI PSRAM trong README gateway. Header giao thức `balance_wire.h` và `wheel_measurement.h` có bản tương ứng ở hai firmware; thay giao thức phải cập nhật cảhai.

Mô phỏng chỉ cần Scilab; entry point nhóm không đọc log cũ. Các giá trị lực/ma sát/trễ là giả định có ghi rõ. Thay số mô phỏng không tự đổi firmware.

## Thu dữ liệu mới theo cách của nhóm

Web lưu mẫu cân bằng100Hz từ controller200Hz/IMU100Hz; TXT27cột có phiên bản, seq, thời gian, CRC/gap/overflow/reset và END. Có tối đa1giây trước ARM và2giây sau STOP. Tải trước khi mất nguồn; đầy bộ nhớ không ghi đè. Mỗi lượt nên mộtfile và ghi rõ có chạm thân xe hay không.

Đo gyro là đo bias lúc đứng yên, không đặt tư thế đó thành0°. Nguồn motor/tải/độ bám/chênh lực hai bánh chưa được đo đầy đủ. Điều khiển tốt trong mô phỏng chưa đủ chứng minh xe thật ổn định.

Các ví dụ PID/giao thức trong tài liệu nền tảng có phần từ phiên bản trước; mã nguồn hiện hành quyết định hành vi thực. Xem `Docs/Thong_So_Xe_Hien_Tai.md` để đối chiếu thiết kế và giả định.
