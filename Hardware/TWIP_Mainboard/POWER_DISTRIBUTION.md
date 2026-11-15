> Bản PCB ngày 06/11/2026 dùng một mặt đồng B.Cu, 31 đoạn dây cầu trong 24 nhóm. Tâm thân BMI160 tại (130,00; 81,65) mm. Xem [SINGLE_LAYER.md](SINGLE_LAYER.md) và [bảng dây cầu](SINGLE_LAYER_WIRES.csv) cho bản lắp hiện hành. Các tọa độ bố trí và số dây của bản trước bên dưới là dữ liệu lịch sử.

# Phân phối nguồn trên PCB — thay WAGO

Bản này thay chức năng phân phối của hai cụm WAGO bằng đường đồng trên carrier160×65 mm,2lớp. Giữ nguyên pinout, net điện, nguồn cấp và firmware của xe đang chạy. Tọa độ bốn lỗ, vị trí/hướng năm module và các tụ không thay đổi trong lần đi dây nguồn này.

## Cách lắp không cần WAGO

- Dây từ pin/đầu XT60 ngoài xe nối vào **J1**, chân1 là BAT+, chân2 là GND/BAT−. Footprint J1 là đầu nối2chân5,08mm, không phải footprint XT60. Chọn đầu nối đúng kích thước và đủ dòng thực tế; chưa có đo dòng để chứng nhận một loại đầu nối cụ thể.
- XL4015 hàn trực tiếp bốn chân IN+/IN−/OUT+/OUT− vào PCB. Đặt đầu ra5,0V trước khi cấp cho các bo.
- ESP32, STM32, A4950 và BMI160 nhận nguồn qua chân cắm đã đi dây. Không cần WAGO chia pin hoặc chia5V ngoài carrier.
- Motor/encoder vẫn nối theo J2/J3, giữ đúng thứ tự dây và cực motor của xe. Không đưa pin3S vào chân5V/3V3.

## Thay thế từng nhánh theo hướng dẫn đi dây

| Chức năng cũ | Thực hiện trên PCB |
|---|---|
| WAGO1 (+) chia pin tới VM driver và IN+ buck | Hai nhánh riêng xuất phát từ J1 chân1; đường mục tiêu1,5mm |
| WAGO1 (−) chia về driver và IN− buck | Đường GND riêng rộng1,5mm từ J1 chân2 tới GND1 driver và IN− buck, cùng vùng GND chung |
| WAGO2 (+) chia5V | OUT+ buck cấp tới chân dương C1; tại vùng C1 tách riêng ba nhánh5V cho ESP32, STM32, VCC driver; nhánh nguồn tới C1 rộng1mm, các nhánh tải0,7mm |
| Tụ tại tải | C4/C5 lấy từ nhánh ESP; C6/C7 từ nhánh STM; hai VCC driver cùng nhánh driver. C2/C3 tại nhánh VM |
| WAGO2 (−) về GND hàng2 driver | Thêm đường hồi GND rộng1mm từ OUT− buck tới GND2 driver, vẫn cùng net GND |
| Nguồn3V3 cảm biến/encoder | Giữ từ STM32 tới BMI160/CS và hai encoder, không cấp5V vào tín hiệu BMI |
| UART, I2C, PWM, encoder | Giữ nguyên đường dây và net; đây không phải các đường cần chia hình sao |

Các nhánh công suất/nguồn logic được đi riêng đến vùng phân phối; không nối tiếp nguồn MCU qua pad nguồn driver. Nhánh cùng tải có thể nối giữa các chân nguồn lặp lại trên cùng module. Các điểm rẽ gần C1 có vùng nối đồng chung, không phải các nguồn cách ly.

## Phạm vi của GND

Tài liệu gốc yêu cầu chung mốc GND và nhánh dây riêng nhằm tránh dùng ray mang dòng motor cho cảm biến. Carrier giữ GND chung trên hai mặt và thêm các đường hồi rộng ở khối công suất. Vì plane chung vẫn nối các khối, **đây không phải star-ground một điểm tuyệt đối**, không thể khẳng định toàn bộ dòng motor chỉ chạy trên một đường. Không cắt plane thành các đảo rồi buộc UART/I2C đi qua khe.

Chưa có sơ đồ nội bộ chính xác của module Dual A4950/XL4015 để coi hai chân GND driver hoặc IN−/OUT− buck là cách ly. Không thêm net-tie, điện trở hoặc ferrite vào GND chỉ để tạo hình sao trên sơ đồ. Không coi kết quả DRC là phép đo ground bounce/EMI.

Nguồn tham khảo nguyên tắc: [TI — Best Practices for Board Layout of Motor Drivers](https://www.ti.com/lit/an/slva959b/slva959b.pdf) về đường hồi, plane và bố trí dòng lớn; [Allegro — A4950 datasheet](https://www.allegromicro.com/~/media/Files/Datasheets/A4950-Datasheet.pdf) về nối mass và tụ gần driver. Hướng dẫn ở cấp IC không chứng minh cách nối nội bộ của module mua sẵn.

## Kiểm tra và giới hạn

Đã kiểm các pad/net không đổi so với bản trước, đường tín hiệu không đổi, DRC/parity/unconnected=0. Các nhánh mới có via xuyên lỗ Ø1,2mm/lỗ0,6mm khi cần chuyển lớp; không phải PCB một lớp. Bề rộng này là lựa chọn bố trí, chưa phải chứng nhận dòng cực đại/nhiệt vì chưa chốt độ dày đồng và đo dòng tải.

Trước khi cho xe chạy: kiểm cực/thông mạch khi tắt nguồn; kiểm5V/3V3 khi chưa nối motor; sau đó đo nguồn tại ESP/STM và VM lúc motor khởi động/phanh/đảo. So sánh log reset/IMU với cấu hình xe cũ. Giữ lối cấp nguồn này nếu thử tải đạt; chưa được coi là đã chứng minh ổn định thực nghiệm chỉ vì giữ cùng schematic.

Cập nhật02/11: các nhánh nguồn đã đi lại sau đổi bố trí cổng nạp; điểm phân phối J1/C1 và net nguồn giữ nguyên. Không dùng số liệu chiều dài đường dây trước02/11 để đánh giá sụt áp bản mới.
