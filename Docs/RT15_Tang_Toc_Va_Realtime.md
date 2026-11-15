# RT15 FAST —03/11/2026

Nền RT14/R12, không viết lại bộ điều khiển. Dữ liệu đầu vào: Downloads/twip_balance_R12 (1) (1).txt và (2) (1).txt.

## Log thực tế

Hai file khai báo RT14/NET13. Vòng điều khiển file1:4999–5010µs, thực thi lớn nhất646µs. Tuổi gói mới tại mẫu khi sequence đổi: trung vị11ms,tối đa20ms. Đó là thời gian từ STM nhận gói tới mẫu, không phải trễ đầu-cuối từ ngón tay. Có UART capture task1ms trên ESP, joystick gửi ngay khi đổi hướng và lặp20ms; UI poll100ms. Không thay I2C sangDMA hoặc tăng tần số cảm biến vì log không chứng minh lỗi nằm ở đó.

72cm/s đã đạt khoảng0,739m/s; MAX có đoạn0,906m/s với duty gần0,988. Giới hạn gia tốc và cờ16384 làm lên tốc chậm; motor gần bão hòa không còn nhiều dự trữ. File1 có lần dừng fault8: sai lệch so điểm giữ vượt ngân sách, không phải hết tổng hành trình60cm. File2 cũng chứa lượt fault8 và lượt STOP. Không nới bảo vệ để che lỗi trôi.

## Sửa và giữ nguyên

Preset72/MAX95 tăng cap governor từ0,35→0,50m/s² dưới/ngang0,50m/s; phía trên0,18→0,22m/s². Nguồn gọi cung cấp rate đủ lớn nhưng cap governor quyết định giá trị thực. Preset50 giữ nguyên. Giữ jerk2m/s³, các gate dự trữ PWM/góc/sai số vận tốc và dwell. RT14 neutral không tăng|vref| được giữ. Không thay CAT_K,observer,allocator,hiệu chuẩn BMI,encoder,PWM hoặc các giới hạn bảo vệ.

Các cách tăng mạnh hơn đã thử bị loại vì có ca kết thúc sớm dù baseline qua. Không chọn chúng để nạp. Phiên bản cuối trùng candidate5, không phải candidate2/3/6 trong thư mục thử nghiệm.

STM: BAL-20261103-R12-8DIR-RT15-FAST. ESP: TWIP-WEB-20261103-BAL-R12-8DIR-RT15-FAST-NET14. Nạp cảhai; phíaESP đổi nhận diện tương thích và headerlog, không thay lệnh giữ/thả/joystick. Bảng testencoder vẫn được gỡ.

## Kiểm chứng bằng Scilab

Chạy baseline và các phương án qua tools/run_governor_sim.cjs, mô hình phi tuyến hiện có, bộ điều khiểnC cùng header firmware. Bản cuối:12/12ca chạy đủ20s và không có guard fault. Các ca gồm50/72/95, tiến/lùi/đảo chiều/chéo,pin11,2V,hệ số bám0,8,chênh motor và nhiễu cảm biến. Đây là mô phỏng có điều kiện với tham số chưa độc lập xác minh; không phải bảo đảm xe thật.

Baseline: Simulation/results/runs/20261103_175700_528_motor_catalog_rt14_stress_reference.

Bản chọn: Simulation/results/runs/20261103_180204_019_motor_catalog_rt15_candidate5_stress.

| Mốc trong ca danh định, tính từ lệnh chạy t=3s | RT14 | RT15 |
|---|---:|---:|
| Preset72 đạt50cm/s |2,47s|1,75s|
| Preset72 đạt68,4cm/s (95% mục tiêu) |4,39s|3,40s|
| Preset95 đạt50cm/s |1,96s|1,77s|
| Preset95 đạt68,4cm/s |2,73s|3,395s|

Không giấu hồi quy: MAX95 không nhanh hơn ở mọi mốc; dự trữ góc khiến đoạn giữa chờ nhiều hơn. Mô hình cảhai không đạt90,25cm/s trong ca95 danh định, đỉnh khoảng0,866→0,871m/s. Vì vậy chỉ có cơ sở tăng trần gia tốc để thử, không khẳng định tăng tốc lên95 đạt hoàn toàn. Ca50 cho chuỗi kết quả giống baseline. Pitch cực đại ca72/95 khoảng7,94°, tương đương baseline7,96°.

Đã kiểm thử native governor: jerk,đạt tham chiếu khi có dự trữ,giữ/đảo chiều,bão hòa,thảnuetral cảhai chiều,nonfinite; các kiểm tra UI/FSM/session/HTTP/log giữ nguyên. Hai firmware phải biên dịch thành công trước khi giao.

## Thử xe và tính năng

Giữ trái đỏ→AOUT2,trắng→AOUT1; phải đỏ→BOUT1,trắng→BOUT2. Không đổi khung/encoder/IMU cùng lúc. Thử50trước,72rồi95, một lượt một preset; chỉ chạy thẳng khi đủ khoảng trống, giảm tốc trước quay. Giữ lệnh đủ lâu để phân biệt ramp và plateau, dừng nếu bất thường. Ghi log tiến/thả,lùi/thả và8hướng; không thửMAX/đẩy mạnh ngay sau nạp. Chưa bảo đảm hoàn toàn mượt trên xe lắp lại.

Danh sách tính năng đầy đủ tại Tinh_Nang_Xe_Hien_Hanh.md. Luồng hiện tại: BMI/encoder→STM200Hz→PWM20kHz, telemetry50Hz→ESP→web; lệnh joystick20ms→ESP→STM, đồng thời các cơ chế an toàn vẫn được giữ.
