# LOAD2 — sửa từ chối thử tốc độ khi xe tự chỉnh cân bằng

Log người dùng: `C:/Users/DONG NHIEN/Downloads/twip_load_speed_R12.txt`,683mẫu. Metadata `$LTEST,4,6,0,0.0000,12.0`: từ chối đầu vào, thời điểm bắt đầu0; **không có mẫu nào mang bit4096 chạy thử**. Mẫu đầu có bit16384 ởSTM32 t286551ms: state3, fault0, góc0,475°, gyro−7,976°/s, bánhtrái0,059411m/s, bánhphải0,046158m/s, vref0. Những mẫu trước đó có bánh0,05–0,073m/s do tự chỉnh cân bằng.

LOAD1 yêu cầu cả hai bánh<0,05m/s suốt1giây, và từ chối một lần sẽ khóa lượt tớiARM mới. Web kiểm theo mẫu thưa hơn STM32 nên có thể hiện nút cho bấm trong khi điều kiện200Hz chưa đạt. Đây là điều kiện bắt đầu quá khắt khe, không phải lỗi BMI160 hay ngắt bảo vệ khi xe đã chạy test trong log này.

LOAD2 sửa riêng phần tiếp nhận:

- Chờ ổn định0,5giây: góc≤5°, gyro<35°/s, mỗi bánh<0,12m/s, điểm đặt vận tốc<0,01m/s; vẫn phải cân bằng ởpha bình thường, sẵn sàng, không giữ hướng/quay và không lỗi.
- Nếu bị từ chối trước khi thực sự chạy, lệnhtrungtính sẽ đưa trạng thái thử về chưa bắt đầu. Web tự nhả khi hết1,5giây chờ xác nhận; có thể đợi nút sáng rồi bấm lại **không cần tắt/bật cân bằng**.
- Một lượt **đã thực sự chạy** vẫn không được tự chạy lại trong cùngARM. Các điều kiện kết thúc, dự phòngPWM, trần vận tốc, thời gian/quãng đường và các bảo vệ nền không nới.
- Ngưỡng0,12m/s chỉ dùng để cho phép bắt đầu trong lúc xe tự chỉnh nhỏ, không phải ngưỡng cắt khi xe đang tăng tốc.

Đã qua5 bộ kiểm thử liên quan: helper, parser/FSM, web, session, gateway. Bổ sung ca bánh0,075/0,065m/s đang chỉnh cân bằng rồi bấm ở0,059411/0,046158m/s: nhận thử. Bánh≥0,12m/s vẫn bị từ chối. Kiểm từ chối→nhả→thử lại, và lượt đã chạy không tự lặp.

Nạp đồng bộ STM32 `BAL-20261031-R12-8DIR-RT9-LOAD2` và ESP32 `TWIP-WEB-20261031-BAL-R12-8DIR-RT9-LOAD2-NET6`; bản mới nằm trong `.build/balance_release_r12_rt9_load2_20261031/`. Mở `http://192.168.4.1/ui/r12?v=rt9-load2-net6` để tải giao diện mới. Chưa có xác nhận trên xe thật choLOAD2.

Tài liệuLOAD1 giữ lại để đối chiếu lịch sử; dùng điều kiện bắt đầu và bản nạpLOAD2 ở đây.

Build cả haiMCU thành công; ESP32 flash1223443byte(38%), RAMtĩnh59404byte(18%). Kiểm trực tiếpBIN xác nhận hai buildIDLOAD2 và điều kiệnweb500ms/0,12m/s. SHA-256 nằm ởmanifest.json trong thư mục bản nạpLOAD2. Dữ liệu50Hz trước lúc bấm đạt điều kiện mới liên tục khoảng2180ms, điều kiện cũ0ms; không suy ra toàn bộ mẫu200Hz chưa ghi từ dữ liệu50Hz này.
