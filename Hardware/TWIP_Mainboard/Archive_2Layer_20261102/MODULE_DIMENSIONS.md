# Kích thước module hiện hành theo tâm chân và mép PCB

Cập nhật02/11/2026. Đơn vị **mm**. Đây là thông số CAD để đối chiếu mẫu thật, không phải chứng nhận kích thước hãng. Các kích thước cũ trong ghi chép lịch sử được thay bằng bảng này.

| Module | Đường bao đang dùng | Cơ sở tính | Chưa xác nhận bằng số đo mép mới |
|---|---|---|---|
| STM32 Black Pill | **52,26 × 20** | 1 +19×2,54 +3 =52,26; giữ bản đã được người dùng xác nhận | Không sửa trong lượt này |
| ESP32, thân có chân | **57,34 × 28** | 2 +21×2,54 +2 =57,34 | Chiều rộng28 và khoảng hai hàng25,5 giữ số cũ |
| ESP32 tính cả anten | **64,34 × 28** dự trữ | 57,34 +7 =64,34 | Bề rộng lưỡi anten vẫn dự trữ28, chưa đo riêng |
| BMI160 | **18,10 × 13** | Khớp gần đúng các mép; hàng5chân lệch dọc−0,40 so hàng7chân | Chiều ngang giữa hàng/chiều cao thân13, khoảng hàng10 giữ số cũ |
| Dual A4950 | **23 × 17,64** theo trục bản vẽ | 1,2 +6×2,54 +1,2 =17,64 theo hàng7chân | Chiều23 vuông góc hàng chân, khoảng hai hàng20 giữ số cũ; người dùng xác nhận1,2 chỉ là mép đầu/cuối hàng |
| XL4015 | **54 × 24**, tâm chân **50 × 20** | 54−2×2=50;24−2×2=20 | Thay tâm chân51×21 cũ; độ cao/đường kính chân vẫn cần kiểm |

## BMI160: không thể khớp tuyệt đối cả bốn số với bước2,54

Hàng7chân: VIN1,8 +6×2,54 +SA01 =**18,04**.

Hàng5chân: OCS4 +4×2,54 +SDX4 =**18,16**.

Chênh0,12mm. Người dùng xác nhận hai hàng có lệch dọc nhẹ và yêu cầu theo số đo. Chọn chiều dài trung gian18,10mm và dịch hàng5chân−0,40mm dọc theo hàng so với hàng7chân. Đường bao x114,05..132,15 cho các khoảng thực hiện:

| Mép đến tâm chân | Đo cung cấp | CAD thực hiện | Sai lệch |
|---|---:|---:|---:|
| VIN |1,80|1,83|+0,03|
| SA0 |1,00|1,03|+0,03|
| OCS |4,00|3,97|−0,03|
| SDX |4,00|3,97|−0,03|

Giữ bước trong từng hàng2,54mm; không kéo giãn để ép mọi khoảng mép cùng đúng. Hàng5chân thay tọa độ dọc0,4mm; đã chỉnh đường INT1 liên quan, tên/net không đổi. Không xoay cảm biến, không đổi quy ước7chân=tiến/5chân=lùi. Tâm thân BMI mới(123,10;82), gốc footprint vẫn(123,5;82), không phải tâm thân.

## Vị trí trong KiCad

- Khung carrier vẫn160×65, góc trên trái(50;50), dưới phải(210;115).
- ESP thân x152,83..210,17/y58,5..86,5; anten x210,17..217,17. Chân cắm ESP giữ nguyên. Khoảng trống trước USB từx133 đến152,83 còn**19,83mm** (xấp xỉ20mm), không có linh kiện khác trong hành lang. Toàn bộ bao ngoài gồm anten vượt cạnh phải7,17mm; không lấy phần này để scale khung carrier.
- A4950 thân x112..135/y94,18..111,82; pad giữ nguyên.
- XL4015 thân x56,5..110,5/y58..82. Tâm IN+ (58,5;60), IN− (58,5;80), OUT+ (108,5;60), OUT− (108,5;80). Đã chỉnh pad và điểm nối dây, không đổi net.
- STM32 giữ nguyên cả hình học và pad đã xác nhận. Các lỗ M3 và tụ không thay đổi trong lần này.

Schematic chỉ cập nhật association U1 sang `TWIP_Carrier:XL4015_Measured_50x20_Edge2`; kết nối điện giữ nguyên. Thư viện footprint local đã đồng bộ. ERC/DRC/parity/unconnected đều0 trên bản kiểm mới; vẫn cần in100% và kiểm mẫu thật, nhất là các chiều còn giữ số cũ.

Không tự thay khoảng cách hai hàng thành bội số2,54 nếu chưa có số đo/đếm bước xác nhận; mặc định2,54 trong yêu cầu này là bước giữa hai chân liên tiếp **trong một hàng**. Hai hàng BMI lệch dọc thì không còn thẳng cột tuyệt đối trên lưới breadboard, dù từng hàng vẫn bước2,54.
