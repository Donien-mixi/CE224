# BMI160 và pipeline STM32 — 2026-11-01

Firmware STM32: `BAL-20261101-R12-IMUFIX-8DIR`. Giữ định dạng gói 8DIR 114 byte, cấu hình IMU 100 Hz, vòng điều khiển 200 Hz và hệ số lọc R12 0.998 tại chu kỳ 5 ms.

## Phát hiện và sửa

- Driver cũ chỉ kiểm tra CHIP_ID, bỏ qua kết quả các lần ghi reset, đánh thức và cấu hình. Nay mọi lần ghi phải thành công; đọc lại ERR_REG, PMU_STATUS (accel và gyro đều normal), CHIP_ID và bốn thanh ghi cấu hình trước khi cho phép dùng số đo. Chờ sau mỗi lần ghi theo giới hạn giao tiếp tại mục 3.2.4 của datasheet BMI160 đi kèm.
- I2C ACK và dữ liệu giống nhau không đủ để kết luận cảm biến còn hoạt động. Đọc STATUS trước burst DATA, theo dõi DRDY accel/gyro riêng; thiếu cập nhật từ một trong hai trong 30 ms làm hàm đọc thất bại. Cho phép các lần đọc trùng giữa hai mẫu 100 Hz và dữ liệu không đổi khi xe đứng yên. Khi mất mẫu, fault 32 được bật và motor dừng; pitch/raw trong telemetry giữ số đo hợp lệ cuối cùng, không phải góc mới. Cần xem cả fault/state khi chẩn đoán góc cố định.
- HAL I2C có nhánh chờ BUSY tới 25 ms dù timeout truyền vào là 2 ms. Driver kiểm tra BUSY trước để tránh nhánh chờ đó trong vòng 5 ms. Timeout trong giao dịch vẫn phụ thuộc SysTick; ưu tiên SysTick 0, TIM4 1 đã có trong Robot_Init được giữ nguyên.
- Bộ lọc cũ trộn tuyến tính hai góc qua ranh giới ±180°, có thể kéo kết quả về gần 0° khi xe lật. Nay dùng sai số góc ngắn nhất và chuẩn hóa đầu ra về [-180°, 180°]. Hệ số theo dt thực giữ cùng đáp ứng thời gian khi khoảng cách ngắt thay đổi. Encoder và tích phân vị trí cũng dùng khoảng cách ngắt đo bằng DWT; bộ điều khiển rời rạc vẫn có chu kỳ thiết kế 5 ms và giữ chốt lỗi timing cũ.
- Vòng main cũ khóa toàn bộ ngắt khi giải mã UART, chặn cả SysTick và DMA. Nay chỉ khóa TIM4 khi cần bảo vệ cấu trúc lệnh. UART/DMA và SysTick tiếp tục chạy. TIM4 vẫn bị trì hoãn trong thời gian parse; số đo spacing/execution trong telemetry cần được kiểm tra thực tế.
- sysmem.c dùng NULL/ptrdiff_t nhưng thiếu stddef.h; đã bổ sung include trực tiếp.
- Lời gọi BalanceMotion8_Update thiếu tham số yaw_rate nên mã bàn giao không biên dịch được. Đã nối vận tốc quay từ encoder `(v_right-v_left)/wheelbase`, cùng quy ước dấu/đơn vị rad/s với bộ điều khiển yaw.
- Hiệu chuẩn kiểm tra cả cập nhật dữ liệu và chuẩn gia tốc; chỉ chấp nhận bias mới khi đủ mẫu đứng yên hợp lệ. Hiệu chuẩn thất bại giữ bias trước và chặn ARM. CALIB khởi tạo lại cảm biến để phục hồi sau reset/suspend, hủy yêu cầu ARM/PULSE/di chuyển đang chờ, bỏ byte UART tồn đọng trước khi mở TIM4 và đặt lại mốc thời gian DWT của vòng điều khiển.
- STOP sau lỗi cảm biến giữ trạng thái EMERGENCY, tránh biến lỗi cảm biến thành FALLEN có thể ARM lại. Nhánh FALLEN bình thường vẫn tiếp tục đọc IMU, tính góc và phát telemetry khi motor dừng.

## Kiểm chứng trên máy

Chạy `tests/stm32/run.sh` từ thư mục gốc. Harness biên dịch driver BMI160, bộ lọc và encoder thật với HAL giả lập; chạy FSM thật với motor/UART giả lập. Mặc định bật UndefinedBehaviorSanitizer. Có thể chọn compiler bằng `CC` và cờ sanitizer bằng `SANITIZERS`.

Đã kiểm tra: địa chỉ 0x68/0x69; lỗi ghi/ghi không có tác dụng; cảm biến còn suspend; ghép số âm và đổi đơn vị; đọc trùng hợp lệ; thiếu DRDY từng trục; tràn HAL tick; BUSY trả lỗi ngay; hiệu chuẩn đứng yên/chuyển động/gia tốc zero; giữ bias khi thất bại; góc tĩnh ±90°; đi qua ±180°; đáp ứng tương đương giữa dt 5/10 ms; ARM rồi STOP; góc tiếp tục đổi sau FALLEN; mất mẫu vào EMERGENCY; STOP giữ EMERGENCY; CALIB phục hồi hoặc thất bại; hủy lệnh cũ; frame mới mang góc hiện tại, PWM zero và CRC hợp lệ.

Toàn bộ 19 file C trong Core/Src cũng được kiểm tra cú pháp với GCC, các header HAL/CMSIS thật và định nghĩa STM32F411xE/USE_HAL_DRIVER. Đây là kiểm tra trên máy, chưa thay thế build/link firmware bằng ARM GCC, nạp board hoặc đo timing trên phần cứng. Không có kết nối board trong phiên sửa này.

## Xác nhận trên xe

1. Build project trong STM32CubeIDE, nạp STM32 và xác nhận build ID mới trong BINFO. Giữ motor tắt, đặt xe yên khi CALIB khoảng 5 giây; kiểm tra không có fault 32/128/256.
2. Nghiêng chậm trước/sau tới 30°, 60°, gần 90°, rồi dựng lại. So sánh raw ax/az, raw gy và pitch; pitch điều khiển dương về phía trước. Dấu gyro Y và hướng lắp vẫn cần đối chiếu trên xe thật. CF 0.998 có hằng số thời gian khoảng 2.5 giây, nên sai lệch tĩnh sau một chuyển động không được gyro ghi nhận có thể mất vài giây để hội tụ.
3. Giữ xe trong gá, ARM rồi STOP; tiếp tục nghiêng xe. PWM phải bằng zero, sequence/t_ms và pitch tiếp tục thay đổi. Khi thử mất IMU trong gá, kiểm tra fault 32 và motor dừng; STOP không xóa EMERGENCY. Kết nối lại và CALIB để phục hồi.
4. Theo dõi dt_us gần 5000, execution_us dưới 4000, fault timing 64 và dropped khi gửi lệnh liên tục. UART 115200 có đủ băng thông danh định cho telemetry 114 byte × 50 Hz; thời gian thực và nhiễu nguồn/I2C vẫn cần đo khi motor chạy.

Log của một manual diagnostic job được cố ý đóng băng khi hoàn tất hoặc hủy; mã không tự mở lại job để tiếp tục lấy góc. Nếu đang xem trang đo phần cứng, cần mở job mới để có dữ liệu mới. Điều này khác với telemetry cân bằng vẫn chạy sau FALLEN.
