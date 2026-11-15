# R12 RT13 RESP95 — đối chiếu log 34/35 và cải thiện khởi hành

Ngày 01/11/2026. Đây là bản chỉnh lớp sinh tham chiếu của R12, không phải thay PID hay đổi cơ khí. Người dùng xác nhận lấy VM = 11,7 V làm điều kiện tham chiếu; firmware chưa đo VM trực tiếp.

## Dữ liệu thực tế

Nguồn: hai file `twip_balance_R12 (34).txt` và `(35).txt` trong Downloads. Cả hai ghi STM32 `BAL-20261031-R12-8DIR-RT12-GOV95`, ESP32 NET9.

Đã giữ bản dữ liệu gốc tại `Simulation/data/raw/20261101_rt12_speed/` để lần sau không phụ thuộc thư mục Downloads; không đưa log vào bản nạp.

| Chỉ tiêu | Log 34 | Log 35 |
|---|---:|---:|
| Số mẫu | 8.534 | 14.030 |
| Khoảng thời gian ghi | 170,66 s | 280,58 s |
| CRC / sequence gaps / overflow / STM32 reset | 0/0/0/0 | 0/0/0/0 |
| Vận tốc tham chiếu lớn nhất khi giữ chạy thẳng | 88,50 cm/s | 87,71 cm/s |
| Đỉnh vận tốc trung bình hai encoder khi giữ chạy thẳng | 93,39 cm/s | 92,98 cm/s |
| Đỉnh vận tốc observer cùng điều kiện | 91,78 cm/s | 91,96 cm/s |
| Số lần vào REC | 2 | 4 |

Trong các mẫu chạy thẳng có |vref| > 0,84 m/s, vận tốc observer trung bình theo trị tuyệt đối khoảng 85,8–85,9 cm/s và PWM lớn nhất trong cặp trung bình khoảng 90% toàn thang. Đây là thống kê có điều kiện, không phải vận tốc trung bình toàn lượt hay phép đo quãng đường độc lập. Không lấy một đỉnh encoder làm bằng chứng giữ ổn định tốc độ đó.

Hai file kết thúc bằng fault 1 và có GSTOP `stop_button_ws`; không có fault IMU/timing trong các mẫu ghi. Các lần REC vẫn cần được xem là tác động bảo vệ thực sự: log 34 có các mẫu vào REC với gyro khoảng 193 và 219°/s; log 35 còn có chênh vactual–vref lớn. Không bỏ REC chỉ vì xe kết thúc bằng STOP. `archive_error=1` vẫn xuất hiện ở thông tin gateway, chưa được xử lý trong thay đổi này; dữ liệu RAM của hai file không ghi overflow/gap.

## Vì sao chưa đạt 95 và vì sao khởi hành chậm

95 cm/s là yêu cầu người dùng chọn. RT12 có governor giữ nguyên lựa chọn nhưng ngừng tăng vref khi PWM lọc >90%, góc dự báo >5° hoặc tham chiếu đi trước vận tốc đo >0,12 m/s. Khi vận tốc cao, PWM đã quanh ngưỡng giữ dự phòng nên vref dừng trước 95. Không thấy bằng chứng mất dữ liệu giải thích hiện tượng này.

Ngưỡng 90% là lựa chọn bảo vệ phần mềm, không phải phép đo chính xác giới hạn vật lý của GA25. Tuy nhiên, các log trước 31–33 đã có bão hòa PWM rồi tăng góc nhanh khi chạy MAX; không đủ cơ sở gỡ dự phòng để ép đủ 95. Hai log mới chứng minh xe chạy tốt hơn với governor, chưa chứng minh 95 ổn định lâu dài.

Ở tốc độ thấp, governor cũ có thể chặn một nhịp chỉnh góc bình thường và chỉ nhả sau 300 ms liên tục |pitch|<3°, |gyro|<15°/s, duty<85%, sai số tốc độ<0,08 m/s. Điều kiện này dễ bị ngắt khi thân xe vẫn đang chỉnh cân bằng. Ví dụ lượt log35 bắt đầu t=437850 ms cần 3,98 s mới lần đầu đạt 50 cm/s và 5,04 s đạt 72 cm/s. Có lượt khác nhanh hơn nhiều, cho thấy độ trễ phụ thuộc trạng thái thân xe, không chỉ dốc vận tốc danh định.

Tuổi lệnh STM32 trong log không cho biết chính xác thời điểm người dùng chạm màn hình. Không dùng dữ liệu này để khẳng định độ trễ Wi-Fi một chiều.

## Thay đổi đã chọn

Chỉ khi yêu cầu 72/MAX95, cùng chiều với vref và |vref|<0,50 m/s:

- Ngưỡng giữ tăng tốc theo góc dự báo đổi 5° → 6°.
- Cho tăng tốc lại sau 100 ms đủ dự phòng: duty<85%, góc dự báo<3,5°, |pitch|<4°, |gyro|<25°/s và sai số tốc độ<0,08 m/s.

Giữ gia tốc cũ 0,35 m/s² dưới 0,50 m/s và 0,18 phía trên cho 72/MAX95. Đã thử tăng mạnh hơn 0,45/0,55 nhưng không chọn: lên tốc độ thấp nhanh có thể gây một lần chờ dài hơn ở vùng chuyển tiếp. Không đổi jerk, ngưỡng PWM90%, giảm tham chiếu khi bão hòa/lean8°, xử lý nhả/đảo chiều, lease350ms, bảo vệ, luật R12, observer, chống xoay, hiệu chuẩn hoặc dựng60°. Chế độ50 có cùng kết quả mô phỏng với bản cũ trong ca đối chiếu.

File hành vi: `Xe_Can_Bang/Core/Inc/balance_governor.h`. Các file build ID, kiểm tra tương thích web/STM32 và header log được đồng bộ sang RT13 RESP95. Các test production đổi metadata giả lập cho đúng build mới; không nới assertion bảo vệ.

## Kiểm chứng mô phỏng ở 11,7 V

Chạy hai bộ đối chiếu cùng plant, seed và lịch lệnh, dùng header governor cũ/mới thực sự được biên dịch vào thư viện native. Không tăng lực motor, sửa khối lượng, trọng tâm, nhiễu hay trễ để ép đạt mục tiêu. Các lượt sàng lọc 12 V không dùng làm kết luận 11,7 V bên dưới.

| Tình huống / lần đầu vượt mức vận tốc thực | RT12 | RT13 |
|---|---:|---:|
| MAX tiến đạt50 | 3,400 s | 1,960 s |
| MAX tiến đạt80 | 4,790 s | 3,560 s |
| MAX lùi đạt50 | 3,365 s | 2,465 s |
| MAX lùi đạt80 | 4,760 s | 4,915 s |
| Preset72 đạt50 | 3,570 s | 2,470 s |
| Preset72 đạt72 | 4,480 s | 4,535 s |

Các thời gian đo từ lệnh chạy ở t=3 s, là lần đầu vượt ngưỡng, không phải thời gian xác lập. Cải thiện rõ ở khởi hành; không cải thiện mọi mốc. Đặc biệt không quảng cáo toàn dải nhanh hơn chỉ từ số đạt50.

12 ca gồm50,72,MAX tiến/lùi, đảo hướng, quẹo, VM11,2 V tiến/lùi, giảm lực motor20%, lệch đặc tính hai bánh ±5%, tăng hằng số thời gian motor và trễ cảm biến. Mỗi ca20 s với lệnh chạy từ3 s, nhả tại15 s; tất cả đủ20 s, trace không fault/REC. Góc thật cực đại toàn bộ khoảng11,03° (bản cũ11,28°); RMS vận tốc1 s cuối lớn nhất khoảng1,29 cm/s. Không gọi đó là đứng yên tuyệt đối. Ca50 cho dữ liệu giống hệt trước/sau.

Trong mô hình, MAX vẫn bị hạn chế trước95. Đặc tính có tải, ma sát và tương tác mặt sàn còn chưa nhận dạng đầy đủ; mô phỏng chưa khớp hoàn toàn vận tốc thực. Kết quả chỉ hỗ trợ thử nghiệm có kiểm soát bản mới, không chứng nhận không ngã hoặc chạy lâu ở95. Không dùng các đoạn log tương lai làm đầu vào trạng thái để giả tạo mô phỏng khép kín.

Tái lập: `node tools/run_governor_sim.cjs --stress`; kết quả trong `Simulation/results/runs/`. Runner lưu header, adapter và lịch chạy trong `inputs`, kiểm tra kết thúc đủ thời gian và không fault. Bộ chạy này là bài kiểm tra governor riêng; không mặc định mọi launcher mô phỏng cũ đều đã gọi governor.

Kết quả chính của lượt này:

- RT12: `20261101_075100_499_motor_catalog_rt12_117v`.
- RT13: `20261101_075050_538_motor_catalog_rt13_117v`.
- Thống kê/input hashes: `Simulation/results/analyses/20261101_rt13/`.

Các kiểm tra phần mềm: governor (gồm hồi dự phòng100/300ms, tách50 và72/MAX, PWM, nhả, đảo hướng, kẹt, NaN), production FSM, recovery, replay38 ảnh chụpREC72 cũ, production dashboard/session/gateway. Replay ảnh chụp không thay thế mô phỏng khép kín. Cả hai MCU được biên dịch lại; bản nạp và dấu nguồn nằm ở thư mục release dưới đây.

Build thành công: STM32 text76.208/data492/bss7.460 byte; ESP32 flash1.074.991 byte (34%), RAM tĩnh54.004 byte (16%). Đã kiểm build ID trong cả hai BIN, header governor mô phỏng trùng nguồn firmware và các header luật R12/dựng/yaw giữ nguyên so với manifest RT12 (chuẩn hóa xuống dòng khi cần). Không nạp vào phần cứng trong phiên này.

## Nạp và lấy log tiếp

STM32: `BAL-20261101-R12-8DIR-RT13-RESP95`.

ESP32: `TWIP-WEB-20261101-BAL-R12-8DIR-RT13-RESP95-NET10`.

Thư mục bản nạp: `.build/balance_release_r12_rt13_resp95_20261101/`. Bản cũ RT12 vẫn giữ riêng. Chưa nạp hoặc thử RT13 trên xe trong lượt này. Người dùng nạp lại cả STM32 và ESP32 từ nguồn hiện tại trong CubeIDE/Arduino; mở `http://192.168.4.1/ui/r12?v=rt13-resp95-net10`.

Lấy riêng lượt72 và lượtMAX trên đoạn thẳng đủ khoảng hãm, giữ8–10 s nếu điều kiện thực tế cho phép, nhả để quan sát hãm rồi STOP/chờ2 s/tải. Tách lượt tiến, lùi, đảo chiều để biết mốc khởi hành. Giữ VM tham chiếu11,7 V như loạt này; gửi TXT và nhận xét xe nhích lúc nhấn, rung, hãm, có REC hay không. Nếu có điện áp đo được thì ghi kèm, không coi nhãn11,7 V trong log là cảm biến nguồn.
