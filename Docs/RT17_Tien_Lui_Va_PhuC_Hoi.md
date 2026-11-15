# RT17 SYNC / NET16 — tiến, lùi và nhận lệnh sau va chạm

Phiên bản nạp thử: **STM32 `BAL-20261103-R12-8DIR-RT17-SYNC`**, **ESP32 `TWIP-WEB-20261103-BAL-R12-8DIR-RT17-SYNC-NET16`**. Đã build; chưa có lượt chạy RT17 trên xe. Không đổi cơ khí, đấu dây, CF, PPR, gain/observer/phân bổ lực R12 hoặc PWM tối đa 2499.

## Bằng chứng từ hai log RT16

Đã đọc hết `(10) (1)` — 3.977.294 byte, chủ yếu tiến; `(11) (1)` — 2.039.315 byte, chủ yếu lùi. Có 5 lượt độc lập:

| File | Bắt đầu theo đồng hồ STM32 | Thời gian cân bằng/chạy | Lỗi kết thúc |
|---|---:|---:|---|
| (10)(1), lượt 1 | 87,005 s | 192,90 s | 4 — góc vượt giới hạn |
| (10)(1), lượt 2 | 287,225 s | 136,54 s | 1 — nút DỪNG |
| (11)(1), lượt 1 | 604,485 s | 6,66 s | 8192 — bảo vệ chuyển động mạnh cũ |
| (11)(1), lượt 2 | 616,305 s | 68,20 s | 8192 — bảo vệ chuyển động mạnh cũ |
| (11)(1), lượt 3 | 685,665 s | 87,86 s | 1 — nút DỪNG |

Không có lỗi CRC, mất khung, tràn log hoặc STM32 reset trong header. Không có lỗi I2C/chu kỳ hoặc lỗi 4096/8 trong năm lượt này. Chu kỳ ghi được khoảng 4.996–5.014 µs; xử lý trong các mẫu đang cân bằng lớn nhất 655 µs. Đây là mẫu 50 Hz của vòng 200 Hz, không chứng minh mọi chu kỳ hoặc toàn bộ đường truyền web đều có cùng độ trễ.

PWM đã tới ±2499 ở cả hai bánh. Tốc độ encoder có đỉnh lùi khoảng 100,57 cm/s — là đỉnh quá độ, không phải tốc độ giữ ổn định. Không có số đo VM/dòng trực tiếp; dùng VM tham chiếu 11,7 V theo thông tin người vận hành.

Ở lượt dài đầu tiên, chỉ xét lệnh thẳng: tiến có cờ giới hạn gia tốc 16384 khoảng **64,5%** số mẫu, lùi **25,4%**. Pitch trung bình tương ứng khoảng **+3,90° / +0,23°**. Các lượt giữ và tốc độ chọn khác nhau nên không lấy trung bình vận tốc toàn file làm phép đo motor đồng điều kiện. Nhưng dữ liệu cho thấy nhiều đoạn **STM32 đã nhận lệnh tiến mà tham chiếu bị ghìm**, không phải cứ xe chạy chậm là web chưa truyền lệnh.

Đoạn 372,345–377,405 s đặc biệt rõ: pitch khoảng −0,55…+0,77°, vị trí lệch điểm giữ chỉ vài mm, lệnh trung tính vẫn được nhận, nhưng cổng nhận lệnh mới còn đóng hơn 5 giây. RT16 bắt lại một lần dwell 500 ms sau khi episode hủy đã hoàn tất; đỉnh gyro ngắn có thể liên tục làm dwell thứ hai trở về 0.

Trong đoạn đứng yên đầu lượt 1, pitch cuối khoảng +1,94°; các đoạn đứng yên sau có thể gần 0° hoặc đổi dấu. Không suy ra từ đó trọng tâm/cơ khí chắc chắn sai. Bộ giới hạn tốc độ đang giả định tư thế cân bằng hiển thị phải gần 0°, trong khi xe thực có mốc hiển thị khác.

Header cuối file (11) ghi bias gyro gần ±6°/s. Từ `gy_raw/16.4 − gyroY_corrected` trong **các mẫu đang chạy**, bias Y thực dùng vẫn khoảng −0,1811°/s. Header cuối phản ánh lần hiệu chuẩn cuối không đạt sau khi dừng; code cũ vẫn công bố trung bình dù từ chối hiệu chuẩn. Không được dùng header đó để kết luận toàn bộ lượt chạy đã trừ bias +6°/s.

## Thay đổi được giữ trong RT17

### Giới hạn tăng tốc có mốc tư thế đo được

Giữ ngưỡng vào hạn chế, duty, độ trễ bám tốc, gia tốc và jerk của RT16. Chỉ học **mốc tư thế cho bộ dự báo dự trữ motor** từ lúc xe tự cân bằng, không có lệnh chạy/quay, không phanh/hủy/REC1, tham chiếu đã về 0 và norm accel bình thường.

Sau 1 s ở trạng thái đủ điều kiện, tích lũy 100 mẫu yên hợp lệ: pitch ≤6°, gyro ≤25°/s, tốc độ tâm ≤0,04 m/s, tốc độ quay ≤0,15 rad/s. Mốc giới hạn ±2°; trung bình dưới 0,5° coi là 0. Mốc đóng băng khi điều khiển, chỉ cập nhật lại khi đủ điều kiện nghỉ. **Không yêu cầu vị trí phải về hoàn hảo điểm đặt**: log đang cân bằng có sai lệch vị trí vài cm nhưng vẫn đứng yên.

Mốc này chỉ trừ trong dự báo giới hạn tăng tốc. Góc trên web, luật lực R12, hiệu chuẩn IMU và giới hạn góc cứng không bị dịch. Với mốc 0, đường giới hạn giữ hành vi RT16. BINFO thêm trường 24/25: `equilibrium_deg / qualified`; web hiển thị trong Chi tiết.

Thêm xử lý đảo chiều: không tiếp tục dùng gia tốc lưu đang làm tham chiếu tăng thêm về hướng cũ sau khi người lái đã đổi hướng. Phanh cơ học vẫn cần thời gian; không cam kết bánh đổi chiều ngay tức thời.

### Bỏ lần chờ lặp sau hủy

Episode hủy do kẹt kết thúc sau **200 ms xác nhận yên**: pitch ≤6°, gyro <25°/s, tốc độ tâm <0,10 m/s, quay <0,30 rad/s, tham chiếu gần 0. Xác nhận này đồng thời hoàn tất readiness, không bắt thêm 500 ms lần nữa.

Vẫn cần **trung tính mới rồi thao tác mới** sau hủy. Lệnh giữ cũ không tự chạy lại. Khi cổng đang đóng, giao diện không lưu một lệnh bấm để tự chạy về sau. Phát lại logic C từ cùng đoạn log 50 Hz cho cổng mở sau khoảng **0,52 s ở RT17**, so với **2,12 s ở RT16**. Đây là phát lại giữ mỗi mẫu trong 4 chu kỳ; không tái tạo các mẫu 200 Hz đã mất và không phải dự đoán thời gian thật của xe.

### Phục hồi và lệnh mới phối hợp với nhau

REC1 vào cùng một quỹ đạo phanh có giới hạn dù người lái đang giữ hay đã nhả. Khi vận tốc tham chiếu về 0, điểm giữ được cố định; tránh trường hợp còn giữ nút khiến điểm đặt bám theo xe mãi, mất phần phản hồi vị trí.

- Thân còn chao mạnh: R12 ưu tiên bắt thăng bằng; hướng lệnh mới vẫn được cập nhật, không xếp hàng các lệnh cũ.
- Trong REC1, khi pitch ≤6°, gyro ≤25°/s, tốc độ và tham chiếu ≤0,35 m/s: lệnh mới nhất còn hiệu lực được thực hiện với trần **25 cm/s**, quay **0,4 rad/s**, tăng tốc ≤0,30 m/s².
- REC1 kết thúc mà người lái vẫn giữ cùng thao tác: trần tạm **50 cm/s**, quay **0,8 rad/s**. Nhả rồi nhấn mới, hoặc đổi hướng mới khi đã ở chế độ thường, khôi phục mức đã chọn. Đây là hạn chế tốc độ sau nhiễu, không phải lưu trạng thái giữ nút; web báo rõ.
- Nhả joystick cập nhật trung tính ngay; không tự phát lại thao tác đã nhả. Lease 350 ms, epoch/sequence và nút DỪNG giữ nguyên.

Phát hiện quá tốc ngoài ý muốn được đưa vào REC1 trước khi vượt trần thường: gần trần 0,8/1,12 m/s, sai lệch tốc độ >0,12 m/s theo hướng quá tốc, hệ thống đã được xác nhận ổn định trước đó. Không tăng lại thời hạn 4 s khi đang có episode phục hồi.

Sự kiện `|gyro|>250°/s` cùng norm accel >2 g hoặc <0,4 g chuyển sang **thử phục hồi có giới hạn**, thay vì cắt motor ngay khi thân còn gần thẳng. **|gyro|>900°/s vẫn dừng cứng**, cùng lỗi IMU, dữ liệu không hợp lệ, giới hạn góc/tốc độ/quãng đường/thời gian. Giới hạn REC1 vẫn 50°, khoảng cách tối đa 1 m, 4 s; không biến khởi động 60° thành cam kết phục hồi va chạm ở 60°.

### Hiệu chuẩn gyro công bố kết quả nguyên vẹn

Lần hiệu chuẩn có chuyển động/thiếu mẫu vẫn khóa ARM, nhưng giữ bias hợp lệ trước đó để chẩn đoán; không ghi đè bằng một trung bình sai gần 6°/s. Các bảo vệ I2C, retry NACK, bus clear khi đã dừng của RT16 được giữ.

## Mô phỏng và kiểm thử

Đã sửa hai điểm trong công cụ mô phỏng: không tự giả định người lái nhả nút mỗi lần REC1; cập nhật lệnh mỗi 20 ms như web hiện tại. Đổi preset không reset bộ governor mỗi chu kỳ; reset chỉ ở đầu lượt. Header được chụp trước biên dịch để một lượt đang chạy không lấy lẫn code mới. Luật R12 và hình học danh định không đổi.

Kết quả kiểm tra cuối:

| Bộ ca | Kết quả | Phạm vi |
|---|---:|---|
| Chạy thường, sai lệch motor/tải, VM 11,2 V | **12/12** | Mỗi ca 20 s, không guard, MAX95 không đạt ở mọi ca |
| Lệch mốc góc ±2°, trọng tâm 3 mm, quay và nhấn–nhả | **12/12** | Mỗi ca 20 s, không guard |
| Xung 0,1–0,4 N × 80 ms | **11/12** | Ca không đạt là chiều tiến ở VM 11,2 V, gần vùng thiếu dự trữ motor |
| Thử thách 1–4 N × 80 ms | **4/12** | Cùng bộ điều kiện RT16 đạt 3/12; nhiều ca vẫn ngã/cắt bảo vệ |

Ở ca chạy 50 cm/s, xô +1 N trong 80 ms tại cao 132,5 mm: RT16 kết thúc sớm do trần tốc độ; RT17 vào REC1 rồi phục hồi, góc thân thật mô hình lớn nhất khoảng 11,71°. Đây là cải thiện trong một điều kiện xác định, **không có nghĩa chịu thêm một lực cố định đó trên xe thật**. Mô-men tải, tiếp xúc, trượt bánh và ma sát còn giả định. Không lấy 24 ca thường đạt để nói mọi va chạm đều đạt.

Đã loại các phương án nới ngưỡng mạnh hoặc tự hạ tham chiếu theo duty vì làm một số ca thường kém hơn bản cũ. Không dùng chúng trong bản nạp này. Bộ gain R12 gốc và frame UART vẫn được kiểm tra giống nhau giữa nguồn mô phỏng/MCU; 122 byte, 40 cột CSV. Không đổi encoder để ép hai bánh hiển thị bằng nhau.

Kiểm thử host gồm bộ tạo tham chiếu, neutral/đảo chiều, không chạy lại lệnh cũ, REC1 nhận lệnh có giới hạn, timeout/giới hạn thật, driver BMI160 và encoder. Kiểm thử toàn parser + FSM còn thực hiện chuỗi kẹt → hủy → trung tính → nhận lệnh lùi mới; va chạm mức thử phục hồi vẫn giữ motor, tốc độ quay cực đoan vẫn cắt. Dashboard/session/HTTP log đều qua. STM32 và ESP32 build thành công, chưa nạp bằng công cụ trong phiên này.

Biểu đồ và số liệu: `output/RT17/log_va_nhan_lenh.png`, `mo_phong_va_gioi_han.png`, `metrics.json`, `replay_summary.json`. Kết quả CLI nằm dưới `Simulation/results/runs/`, nhãn `rt17_final_stress`, `rt17_release_edges`, `rt17_final_smallpush`, `rt17_final_challenge` và `rt17_rt16_final_challenge`.

## Nạp và đối chiếu trên xe

1. Nạp cả project `Xe_Can_Bang` và sketch `ESP32_S3_Gateway/ESP32_S3_Gateway.ino`. Web phải báo **RT17-SYNC / NET16**, nếu còn RT16 thì chưa đúng cặp firmware/trang.
2. Kết nối Wi-Fi `TWIP_RACER_S3`, mở `http://192.168.4.1/ui/r12?v=RT17-NET16`.
3. Đo lại gyro khi xe cố định, bật cân bằng trên sàn. Thả tay cho xe tự cân bằng khoảng 2–3 s trước khi điều khiển; trong Chi tiết xem mốc tư thế đã đo. Không ép góc web về 0 hoặc giữ tay làm mốc học.
4. Trên cùng mặt sàn, cùng mức 50: chạy tiến/lùi thẳng với thời gian giữ tương đương, nhả rõ giữa hai lượt. Sau đó thử quay/chéo. Nếu ổn mới tăng 72/95. Tốc độ cơ học không thể bằng nhau tuyệt đối ở mọi tải/độ trượt; cần log RT17 để đánh giá độ lệch còn lại, không cam kết từ code.
5. Với phục hồi, thử xô nhẹ ở tốc độ thấp trước; quan sát web nhận hướng mới, có hiện giới hạn sau nhiễu hay không, nhả có về trung tính không. Không dùng va tường ở MAX95 làm bài xác nhận đầu tiên: các ca thiếu dự trữ đã không đạt trong mô hình.
6. Khi dừng, chờ 2 s rồi tải log trước khi cắt nguồn/đo lại gyro. Ghi ngắn những thời điểm xô/chạm vật, hướng đang giữ, đã nhả hay chưa. Frame có hướng/sequence/thời điểm STM32 nhận lệnh, trạng thái phục hồi và cờ 32768 để tách nguyên nhân.

Không thể bảo đảm vừa đạt tốc độ sát giới hạn motor vừa chống mọi lực xô, mọi dốc hoặc mọi góc. Đổi mới của RT17 là sửa đường nhận/thoát lệnh và tăng khả năng thử bắt lại trong giới hạn còn khả thi; không bỏ bảo vệ hoặc tạo số đo giả để gọi là hoàn thành.

## Nguồn đối chiếu

- [MIT — động lực học cart-pole và giới hạn khả năng tác động theo góc](https://underactuated.mit.edu/acrobot.html).
- [Nghiên cứu TWIP về bù nhiễu và mô hình phi tuyến](https://www.mdpi.com/2076-0825/11/11/339): tham khảo nguyên tắc phân biệt giữ tư thế và nhiễu; không tuyên bố đã triển khai NDOB của bài này.
- [Bosch BMI160 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi160-ds000.pdf): đối chiếu dữ liệu gyro/accel, dải đo và hiệu chuẩn.
