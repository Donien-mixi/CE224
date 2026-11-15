# R12 RT18-LAUNCH / NET17 — sửa khởi hành và hủy lệnh nhầm

Bản nạp thử ngày03/11/2026: STM32 `BAL-20261103-R12-8DIR-RT18-LAUNCH`, ESP32 `TWIP-WEB-20261103-BAL-R12-8DIR-RT18-LAUNCH-NET17`. Chưa có log xe thật RT18. Sửa trên RT17 hiện hành, không thay toàn bộ hệ thống.

## Lỗi được sửa

Nguồn: log `twip_balance_R12 (12) (2).txt`, người dùng xác nhận sàn trống, không va vật cản và không tác động tay. Xem `RT17_Log12_Chan_Doan_Lenh_Tien.md` để đọc đủ4lượt và7lần lệnh tiến bị hủy nhầm.

RT17 yêu cầu vị trí phải cách mốc ARM dưới8cm mới mở cổng điều khiển. Xe đã đứng cân bằng nhưng nghỉ ở−11cm nên có một lượt gần58giây không mở cổng. Khi đã điều khiển, vận tốc bánh có thể còn âm trong pha tạo nghiêng để chạy tiến. Governor giữ tham chiếu nhỏ; sai lệch vị trí hơn6cm cùng vận tốc dưới5cm/s lại bị đếm300ms rồi gán kẹt. Lệnh bị hủy ngay khi bánh đang bắt đầu đổi sang tiến.

### Sẵn sàng sau dựng dựa vào trạng thái yên

Khi chưa thực hiện thao tác chạy/quay đầu tiên, bỏ điều kiện phải về trong8cm so với ARM. Vẫn cần500ms pitch≤5°, gyro<25°/s, tốc độ tâm<0,15m/s, tham chiếu gần0, không bị khóa bởi startup/REC1/cancel và không có hướng tiến/lùi đang xếp chờ.

Không dời mốc giữ khi đang nghỉ, không reset encoder hoặc observer. Thao tác mới dùng cơ chế sẵn có để chốt điểm bắt đầu đúng một lần. Các giới hạn góc, quãng đường, tốc độ của FSM vẫn độc lập; thay đổi này không cho phép bỏ qua giới hạn của pha dựng.

### Phân biệt quá độ khởi hành với kẹt kéo dài

Giữ bằng chứng cơ bản: đang yêu cầu tiến/lùi, điểm đặt dẫn trước vị trí theo hướng lệnh hơn6cm, vận tốc bánh dưới5cm/s. Thay thời gian xác nhận:

- Tham chiếu có độ lớn≥0,30m/s: giữ300ms.
- Nếu chưa đạt mức trên nhưng tham chiếu cùng hướng lệnh≥0,15m/s:1giây.
- Tham chiếu nhỏ hơn hoặc còn ngược hướng ở giai đoạn phanh/đảo chiều:1,5giây, trừ trường hợp độ lớn tham chiếu đã≥0,30m/s áp dụng quy tắc đầu.
- Điểm đặt dẫn trước hơn30cm vẫn là bằng chứng mạnh, hủy trong tối đa100ms. Bằng chứng có sẵn trước đó có thể làm hủy sớm hơn.

Không tắt phát hiện kẹt khi tham chiếu bị governor ghìm rất nhỏ. Đã thêm ca kiểm thử tham chiếu0,04m/s cố định và bánh không quay: vẫn hủy sau khoảng1,5giây xác nhận. Đây là thử logic, không phải phép đo mô-men motor hoặc thử va tường vật lý.

Hủy thật vẫn cần trung tính mới rồi thao tác mới. Giữ nút cũ hoặc gửi thêm packet không tự chạy lại sau khi cổng mở. Nhả nút, lease350ms, epoch/sequence, STOP, giới hạn phanh/hủy và REC1 giữ nguyên.

## Phương án đã loại

Đã thử bỏ giới hạn chênh vận tốc0,12m/s trong một vùng khởi hành nhỏ để tham chiếu tăng nhanh hơn. Dù phát lại log đẹp hơn, mô phỏng thường ở VM11,2V chỉ đạt10/12, giảm so với RT17. Phương án này **không nằm trong firmware giao nạp**.

`balance_governor.h` cuối cùng giống từng byte với RT17 trước phiên sửa này. Ngưỡng duty/góc/chênh tốc, gia tốc, jerk, cơ chế học tư thế của governor không đổi. Bản giao xử lý vòng lặp lỗi bằng cách bỏ hủy nhầm quá sớm, không cố ép motor dương hoặc che phản hồi encoder. Nhích ngược ngắn do tạo tư thế nghiêng vẫn có thể xảy ra; không cam kết chuyển động cơ học đổi chiều tức thời.

## Kiểm chứng

### Phát lại log thật — kiểm tra logic

Dùng cùng các mẫu50Hz, giữ mỗi mẫu4chu kỳ5ms. Không mô phỏng lực tay hoặc tạo lại quỹ đạo vật lý mới.

| Nội dung | RT17 | RT18 |
|---|---:|---:|
|7đoạn khởi hành bị gán kẹt|7đoạn hủy|0đoạn hủy trong khoảng phát lại|
|Đoạn nghỉ lệch11cm|Không mở cổng trong4s được phát lại|Mở khoảng0,56s|

Không dùng0,56s làm cam kết độ trễ thực tế từ joystick tới bánh. Có các chu kỳ200Hz không được lưu trong CSV. Các chuyển động sau khi tham chiếu thay đổi cần kiểm tra bằng mô hình hoặc log xe mới.

### Mô phỏng chuyển động Scilab

Header C được chụp trước khi biên dịch wrapper; cùng bộ R12 với firmware. Không đổi cơ khí, dấu cảm biến, motor, PPR hoặc gain để làm mô hình đạt.

| Bộ cuối | Kết quả | Điều kiện |
|---|---:|---|
|`rt18_final_stress`|12/12|50/72/95, hai hướng, đảo chiều, đường chéo, VM11,2/11,7V và sai lệch motor/tải|
|`rt18_final_edges`|12/12|Nghỉ, sai lệch góc±2°, CG3mm, quay, nhấn–nhả ngắn|
|`rt18_final_launch50`|12/12|50cm/s, sai lệch mốc±2/±3/±4°, nhấn0,8s, nhả, giữ3s, đảo chiều; có ca VM11,2V|
|`rt18_final_highbias95`|1/2|Thử thêm MAX95 với sai lệch mốc lớn; một ca không đạt|

36ca trong ba bộ đầu đều hoàn tất20giây và không cờ guard. Bộ khởi hành50 còn được kiểm tra đã thực hiện đúng hướng yêu cầu và có tham chiếu cùng chiều vượt20cm/s trong lượt giữ9–12s; không tính xe đứng yên vì không nhận lệnh là ca đạt.

Ca chưa đạt là MAX95 với sai lệch mốc+3°: mất ổn định sau khi nhả lệnh, mô hình dừng khoảng12,70s, góc thân thật đạt khoảng65,86°. Cùng ca này RT17 cũng không đạt. Đây là **sai lệch mô hình/cảm biến giả định**, không phải góc nghiêng khởi động3° và cũng không phải nói mọi xe nghiêng3° đều ngã. Nó cho thấy chưa xác nhận được MAX95 trong toàn bộ sai lệch mô hình lớn. Không dùng36/36để tuyên bố mọi va chạm hoặc mọi mức tốc độ đều bảo đảm ổn định. Chưa kiểm thử lại toàn bộ lực xô trong phiên này; không tuyên bố tăng lực chịu đựng.

Kết quả nằm trong `output/RT18/simulation_metrics.json`, biểu đồ `phat_lai_log.png`, `mo_phong_va_gioi_han.png`. Thư mục run đầy đủ dưới `Simulation/results/runs/`, tên nhãn như bảng. Phiên bản thử đã loại và RT17 đối chứng vẫn được giữ riêng trong kết quả, không dùng để nạp.

### Kiểm thử mã và biên dịch

11bộ host kiểm tra header sản phẩm, parser+FSM thật với phần cứng giả lập, BMI160, encoder, web/session/HTTP. Bộ mới `tools/test_rt18_launch.cjs` kiểm tra cả hai dấu: đứng lệch11cm, không tự dời mốc, không nhận lệnh giữ xếp chờ, kẹt thật, tham chiếu cực nhỏ, chênh vị trí mạnh, neutral/nhấn lại. Kiểm thử mới thất bại khi dùng header RT17 đối chứng, đạt với RT18.

Đã build STM32 và ESP32. Artifact được gom trong `.build/balance_release_r12_rt18_launch_20261103/`, kèm SHA256 để đối chiếu. Chưa nạp xuống phần cứng bằng công cụ trong phiên này.

## Nạp và test thực tế

1. Nạp project `Xe_Can_Bang` trong STM32CubeIDE và sketch `ESP32_S3_Gateway/ESP32_S3_Gateway.ino` trong Arduino như trước. Cần đúng cả hai bản vì web kiểm tra phiên bản STM32. OPI PSRAM, flash16MB, partition3MBapp/9MBFAT giữ cấu hình đã dùng.
2. Kết nối `TWIP_RACER_S3`, mở `http://192.168.4.1/ui/r12?v=RT18-NET17`. Kiểm tra tên RT18-LAUNCH / NET17 trên trang. Không dùng lại tab RT17 đang giữ quyền điều khiển.
3. Đo gyro khi xe được cố định. Đặt sàn trống, bật cân bằng, thả tay, để xe tự ổn định2–3giây.
4. Chọn50: giữ tiến khoảng2–3giây, nhả cho dừng; giữ lùi tương tự. Sau đó thử nhấn ngắn0,5–0,8giây, đảo hướng và các hướng quay/chéo. Không giữ tay vào thân khi motor đang chạy trong lượt so sánh.
5. Quan sát hướng lệnh, vận tốc đặt/thực và cờ trong Chi tiết. Cần phân biệt nhích ngược ngắn để tạo nghiêng với việc lệnh tự bị hủy và không thể chạy tiếp. Giữ trang điều khiển sáng và ở phía trước; rời trang vẫn gửi STOP theo thiết kế.
6. Nếu50đã ổn mới thử72 rồi95 trên khoảng trống phù hợp. Không dùng MAX95 va tường để xác nhận đầu tiên. Mức95 vẫn được giữ trong firmware, không phải đã được chứng nhận ở mọi sai lệch mốc lớn.
7. Bấm DỪNG, chờ2giây, tải log trước khi reset/cắt pin/đo gyro lại. Gửi log RT18 kèm hướng nào còn nhích ngược lâu, mức tốc độ và thao tác giữ hay nhả.

Các chức năng R12 trước đó tiếp tục có trong mã: tự cân bằng, cho ARM trong±60° khi đủ điều kiện,8hướng liên tục,50/72/95, nhả/phanh/giữ mốc mới, bù lệch hai bánh/yaw, REC1 có giới hạn, bảo vệ IMU/chu kỳ và ghi/tải log. Việc giữ chức năng không đồng nghĩa đã kiểm thử xe thật lại toàn bộ trong phiên này.
