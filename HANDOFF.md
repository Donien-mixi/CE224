# Bàn giao đồ án xe hai bánh tự cân bằng

Cập nhật bàn giao: **30/10/2026**, múi giờ Asia/Saigon. Nội dung kỹ thuật nền được lập ngày25/10, kiểm tra lại các điểm nhận diện nguồn ngày30/10. Workspace: `D:\DA_HTN`.
Tài liệu được đối chiếu với mã đang có trên laptop, không chỉ tóm tắt lời trao đổi. Các đường dẫn bên dưới tương đối với thư mục gốc project để có thể chuyển máy.

## 0. Bắt đầu phiên mới tại đây — 30/10/2026

**Đúng project: xe hai bánh tự cân bằng tại `D:\DA_HTN`.** File này là điểm vào bàn giao hiện hành, được cập nhật tại chỗ để không sinh thêm nhiều bản handoff khó phân biệt.

### Cập nhật đồng bộ source lên main — 30/10/2026

Lượt bàn giao tiếp theo đã được người dùng yêu cầu đưa firmware RT8/NET5 và mã mô phỏng hiện có lên GitHub main, sau commit Hardware `d6a14af`. Đã chạy 9 CJS và build STM32/native thành công. Xem commit đồng bộ RT8 trong `git log` để xác định HEAD; các nhắc tới `599f472` và working tree RT8 chưa commit phía dưới là mốc bàn giao trước đợt push này. Không coi source đã push là firmware đã nạp hoặc thử xe.

Hardware hiện có KiCad revision 1.3, sơ đồ kết nối module kèm C1–C9 sẽ lắp trên PCB mới. Người dùng xác nhận INT1 → PB2 và còi hai chân (+) → PB12, (−) → GND; breadboard chưa lắp tụ ngoài. Chưa vẽ PCB/chưa gán footprint module.

### Tóm tắt trạng thái trước đợt đồng bộ để không làm lại nhầm việc

- Source local vẫn là **STM32 R12 RT8 / ESP32 RT8 NET5**. Đã đối chiếu build ID trực tiếp trong hai header ngày30/10.
- Kiểm lại7 dấu SHA-256 ở cuối tài liệu: tất cả còn khớp. Điều đó chỉ xác nhận7file nguồn đã ghi dấu không đổi, không phải kiểm toàn repo hay chứng nhận firmware trên xe đã nạp đúng.
- Nhánh local `main`, HEAD vẫn `599f472` (RT4 NET1). Các sửa RT5–RT8 và dọn file còn ở working tree; **bản local mới hơn commit hiện có**. Không có fetch/push trong lượt bàn giao30/10, nên trạng thái remote mới nhất chưa được kiểm lại.
- Còn bản nạp RT8 và RT7 dự phòng trong `.build/`, CSV replay27, công cụ kiểm thử RT8 và đường vào Scilab. Không được coi tên file binary là bằng chứng nó khớp mọi source vừa sửa: build/kiểm hash trước khi phát hành.
- Không có log RT8 mới hoặc xác nhận xe thực tế mới trong phần hội thoại xe sau lần bàn giao25/10. Không đổi trạng thái ba bug thành đã sửa dứt điểm trên xe chỉ vì test/replay đã pass.
- Phiên30/10 chỉ đọc kiểm tra và cập nhật **HANDOFF.md**. Không sửa firmware, không chạy lại train/test/mô phỏng, không build/nạp, không dọn thêm hoặc commit/push.

### Phần chat PROJECT_9 không thuộc xe

Sau nội dung xe, người dùng từng gửi nhầm ZIP Colab nhận diện cử chỉ tay vào chat này. Trợ lý đã chuyển workdir sang `D:\PROJECT_9_NHAN_DIEN_CU_CHI_TAY` cho công việc đó. Kết quả E3/crop1.25/detector300epoch, quotaT4 và notebook INT8 **không phải chức năng đang triển khai trên xe**.

Bàn giao của phần chat nhầm nằm riêng tại:
`D:\PROJECT_9_NHAN_DIEN_CU_CHI_TAY\HANDOFF_P9_E3_FROM_DA_HTN_20261025.md`.
Không tiếp tục phần Colab từ handoff xe, không áp kết quả cử chỉ tay vào firmware xe, và không dọn hoặc sửa thư mục PROJECT_9 khi người dùng chỉ yêu cầu DA_HTN.

### Lệnh khởi động có thể gửi cho phiên mới

> Đọc HANDOFF.md trong D:\DA_HTN để nắm đồ án xe cân bằng, đặc biệt mục0/2/5/6/11/13. Đối chiếu build ID, git status và mã hiện hành trước khi làm. Giữ thay đổi chưa commit, dùng R12 RT8/NET5 làm nền; không quay lại R7, không lẫn PROJECT_9. Phân biệt kết quả replay/mô phỏng với xác nhận trên xe. Tiếp tục theo yêu cầu mới của tôi và báo rõ phần còn chưa được kiểm chứng.

## 1. Đọc trước khi làm tiếp

- **Nền hiện tại là R12, STM32 RT8 và ESP32 NET5.** Không quay về R7 hoặc PID cascade cũ để phát triển mặc định.
- Xe cơ khí DIY đã được người dùng chốt kích thước/bố trí. Không tự thay khối lượng, trọng tâm, lực motor hay ma sát để làm kết quả mô phỏng đẹp hơn.
- Người dùng đánh giá giai đoạn 1 tự cân bằng đã hoạt động tốt; giai đoạn 2 điều khiển web 8 hướng đã có. Ưu tiên hiện nay: nhấn/giữ/thả đúng ý, tiến/lùi và trái/phải đồng đều, ít ngắt do phục hồi nhầm, kết nối bền, tăng tốc nếu đủ bằng chứng.
- **RT8 mới nhất chưa có xác nhận thử thực tế sau sửa trong lịch sử được bàn giao.** Các test/replay trên laptop không thay thế thử xe thật.
- R12 hiện là phản hồi trạng thái + observer + phân bổ lực từng bánh, **không phải bộ năm số PID cũ**. Sửa `pid.c` hoặc các `BALANCE_KP_*` cũ không đồng nghĩa tuning luật R12.
- Mã nguồn quyết định hành vi hiện tại. Nhiều README/tài liệu vẫn xen nội dung lịch sử. Đọc mục 11 trước khi tin thông số trong tài liệu cũ.
- Phiên làm việc này chỉ tạo `HANDOFF.md`; không sửa firmware, không nạp xe, không commit/push.

## 2. Trạng thái mã nguồn và Git

| Mục | Trạng thái khi bàn giao |
|---|---|
| STM32 | `BAL-20261024-R12-8DIR-RT8` trong `Xe_Can_Bang/Core/Inc/balance_profile.h` |
| ESP32 | `TWIP-WEB-20261024-BAL-R12-8DIR-RT8-NET5` trong `ESP32_S3_Gateway/firmware_build.h` |
| Web | `http://192.168.4.1/ui/r12?v=rt8-net5`; tham số v chống cache, không chọn luật điều khiển |
| Wi-Fi | `TWIP_RACER_S3`, mật khẩu `12345678`, AP cục bộ không có Internet |
| Git | Nhánh `main`; origin `https://github.com/Donien-mixi/CE224.git` |
| Commit hiện hành | Xem `git log -1`; `599f472` là mốc RT4 trước đợt đồng bộ RT8 |
| RT8 trên laptop | Được chọn vào commit đồng bộ source RT8/NET5 theo yêu cầu mới; kiểm tra remote để xác nhận push |

Cần chạy `git status`/`git diff` trước khi sửa. Working tree có cả thay đổi firmware cần giữ, thay đổi tài liệu/IDE riêng và các file lịch sử bị xóa qua dọn dẹp. File Excel `Docs/linh kiện hệ thống nhúng.xlsx` đã ở trạng thái deleted từ trước các đợt dọn gần đây; không tự nhận là vừa xóa hoặc tự phục hồi. Không reset/checkout toàn bộ. Không `git add .` khi bàn giao lên Git.

Khi được yêu cầu push: chọn rõ mã để build/nạp, mô phỏng tổng quan và tài liệu cần dùng; không đưa raw log, kết quả AI, binary, cache, metadata IDE hay thư mục `.build` lên repo. Không force-push. Các quyền push/nạp của những lượt cũ không phải yêu cầu tự động thực hiện trong lượt mới.

## 3. Phần cứng, cơ khí và quy ước đã biết

- STM32F411CEU6 Black Pill, Cortex-M4F 100 MHz; ESP32-S3 N16R8; BMI160; hai GA25-370 12 V 280 rpm danh nghĩa, hộp số 21,3:1, encoder AB; driver đôi A4950.
- Pin LiPo 3S cấp VM driver gần điện áp pin. XL4015 hạ 5 V cấp mạch số. Người dùng từng đo VM khoảng **11,7 V** (một đợt 11,68 V); thường sạc khi khoảng 11,2 V. Đây là thông tin lịch sử, **không có ADC pin/dòng/VM trực tiếp trong log hiện tại**.
- Tổng khối lượng mô hình **888 g**; thân gồm motor 788 g, bánh/khớp 100 g; motor 294 g/cặp, 147 g/motor. Đừng dùng lại tổng 794 g hoặc motor 100 g mỗi chiếc từ bản cũ.
- Ba mica 80 × 200 × 1,6 mm; khoảng cách mặt trên 55 mm. Cao mặt trên mica so với trục: 22,5 / 77,5 / 132,5 mm. Bánh đường kính **65 mm**, bán kính lăn **32,5 mm**. Số đo trục khoảng 35 mm trước đây không phải lý do tăng bán kính lăn thành 35 mm.
- Pin/XL4015 ở cụm dưới; breadboard, hai MCU, A4950 và BMI160 ở cụm trên. Người dùng xác nhận **A4950 trên breadboard tầng 2, cọc 55 mm**, thay cho sơ đồ tầng 1/cọc 6 cm cũ.
- **WAGO phân phối nguồn pin là phía trước (+x); WAGO phân phối 5 V là phía sau (−x).** Trái/phải theo người nhìn từ phía sau về phía trước. Mô hình hình học dùng y dương về bên phải theo vị trí các cụm trong mã; đọc bảng tọa độ trước khi diễn giải dấu yaw.
- Người dùng xác nhận PWM dương ở hai bánh làm **đỉnh bánh quay về WAGO nguồn pin**; các nút thử TRÁI/PHẢI đúng nhãn.
- Pitch thô/web ở các bản test IMU cũ **giảm âm khi nghiêng tới trước**. Firmware R12 hiện chuyển sang pitch điều khiển dương về trước. Không suy ra cần đảo motor/gyro chỉ từ dấu góc trong log cũ.
- Các tư thế +Y/−Y trên module từng ngược dấu giá trị raw trên web. Hệ số gia tốc là hiệu chuẩn theo dữ liệu thực; không tự ép dấu để mô phỏng ổn.
- Trọng tâm/quán tính **tính từ bố trí**, chưa đo độc lập: thân z≈36,933 mm trên trục; toàn xe z≈32,774 mm; I thân quanh CG≈0,001368297 kg·m², quanh trục≈0,002443180 kg·m². x_CG=0 là giả định; chưa thể kết luận khung lệch từ việc xe ngã một phía. Sau đó cũng có log ngã cả trước lẫn sau.
- Khoảng cách tâm bánh trong mô hình **262 mm**, suy từ hình học, chưa đo xác nhận. Không nhầm chiều ngang mica 200 mm thành wheel track.

Nguồn đầu vào còn giữ: `thông số chi tiết linh kiện.txt`, `thông số cần thiết.txt`, `thông số cần thiết 1.txt`, `thông số động cơ.txt`, `thông số tổng quan của xe mô phỏng.txt`, `bst-bmi160-ds000.pdf`, `Huong_Dan_Di_Day_Toan_Bo_He_Thong.md`, `Docs/Phan_Cung_Va_Ket_Noi.md`. Có số cũ/mâu thuẫn giữa chúng: khi sửa hình học phải đối chiếu **các phép tính trong `Simulation/model/robot_can_bang_pid_1.sce`**, xác định tọa độ đáy hay tọa độ tâm; không chép máy móc một bảng TXT.

### Chân kết nối chính

| Kết nối | Chân |
|---|---|
| STM32 TX → ESP RX | PB6 USART1 → GPIO18 Serial1 |
| ESP TX → STM32 RX | GPIO17 Serial1 → PB7 USART1 |
| UART | 115200, 8N1, chung GND; STM32 RX DMA vòng/TX DMA |
| BMI160 I2C1 | PB8 SCL, PB9 SDA, 400 kHz, 3V3; địa chỉ thường 0x68, mã có thử 0x69 |
| Encoder trái | PA0/PA1, TIM2 |
| Encoder phải | PB4/PB5, TIM3 |
| PWM A4950 trái/phải | PA8/PA9 và PA10/PA11, TIM1 |
| Còi / LED | PB12 / PC13 |

Các ví dụ EXTI/INT1 hoặc I2C DMA trong tài liệu/nhánh `pipeline` không chứng minh mã local đang dùng chúng. **Driver BMI160 local hiện đọc I2C bằng HAL blocking timeout 2 ms**, chu kỳ cân bằng TIM4 200 Hz; chưa tích hợp mặc định phương án pipeline của đồng đội.

## 4. Luật điều khiển và dữ liệu hiệu chuẩn hiện hành

Luồng STM32: TIM4 5 ms → đọc IMU/encoder → lọc và observer → giám sát khởi động/phục hồi/chuyển động/quay → lực R12 → phân bổ lực/PWM từng bánh → telemetry.

| Tham số | Giá trị/nguồn |
|---|---|
| Điều khiển / IMU ODR | 200 Hz / 100 Hz, không đồng bộ; không gọi log 50 Hz là IMU 50 Hz |
| BMI160 | accel ±8 g, 4096 LSB/g; gyro ±2000°/s, 16,4 LSB/(°/s) |
| Lọc góc | complementary alpha 0,998; zero lắp 2,16694264915°; không tự thay bằng alpha0,98 |
| Encoder đo 10 vòng | L 898,450; R 898,000 ticks/vòng; firmware dùng trung bình **898,225**, không dùng 937,2 danh nghĩa |
| Encoder speed LPF | beta 0,75; code đảo dấu encoder phải; raw ticks phải trong frame vẫn là dấu phần cứng |
| PWM | ±2499 |
| Gain lực R12 | K=[9,150813420; 6,128855436; 14,32794546; 1,160483551], thứ tự x,v,theta,omega; SI m,m/s,rad,rad/s → N |
| Observer | 14 trạng thái, blend0,25; matrix trong `phase1_catalog_params.h` |
| Yaw | PI/PD giới hạn mô-men 0,08 Nm, tích phân 0,04 Nm; điều khiển tốc độ quay khi giữ, hãm rồi giữ hướng khi thả |

Hiệu chỉnh accel `(raw_g − bias)/scale`: bias X/Y/Z = −0,0416836098593 / −0,105884356548 / +0,004738195334 g; scale = 0,997272837552 / 1,00758977736 / 1,00929388014. Có sai số dư/độ bất định tư thế, không tuyên bố phép hiệu chuẩn đã xác minh vật lý hoàn toàn.

`CAT_VNL`, `CAT_FRICTION`, `CAT_START` bù riêng hai bánh/hai chiều. Bảng PWM khởi quay L+/L−/R+/R− là 160/200/200/140. **Không cộng thêm deadband PID lên đầu ra R12**. Không chỉnh bảng motor hoặc encoder để che lỗi governor. Mô-men tải, ma sát, độ trượt và hằng số đáp ứng motor chưa đo độc lập đầy đủ.

## 5. Hành vi người dùng và các giới hạn cần giữ

- Xe không tự chạy motor khi vừa cấp pin. Đo gyro phải đứng yên, không tự ARM sau hiệu chuẩn. Test kê cao dùng pulse ±600 tối đa1,5 s, không phải bài cân bằng.
- Trên nền: gyro/xe yên khoảng200 ms; ARM cho |pitch|≤60°, |gyro|≤6°/s, |v bánh|≤0,05 m/s. Người dùng đã báo nhiều đợt dựng góc lớn thành công, thả tay ngay khi bánh chạy, VM≈11,7 V. Đây là quan sát thực tế của nền R12, không phải chứng nhận RT8 ở mọi điều kiện.
- Pha dựng bắt đầu nếu |góc ARM|>20°. Ngân sách sai lệch `min(1,max(.3,.3+.014*(abs(angle)-10)))` m, tối đa1m. Đây là sai lệch so với tham chiếu/ngân sách pha, **không phải tổng quãng đường điều khiển liên tục**.
- Timeout dựng3s; chỉ ARM>58° mới4s. Giới hạn tốc độ dựng1,00m/s qua48°, 1,10m/s qua58°, 1,20m/s trên58°. Dựng kết thúc sau200ms yên: |pitch|≤8°, gyro<25°/s, speed<0,15m/s. Khả năng dựng từ đứng yên60° không chứng minh cứu được xe đang ngã động60° hoặc đi dốc60°.
- Cân bằng thường: góc dừng25°, tốc độ0,80m/s; ngân sách pha khởi động có thể còn phụ thuộc góc ARM, đọc mã thay vì luôn gán0,30m.
- REC1: sau đủ0,5s ổn; tilt trigger8° giữ yên/12° khi có maneuver; sai lệch tốc độ0,35m/s; với maneuver, tilt/speed cần12tick≈60ms. **RT8 gyro trigger150°/s**, tức thời; không còn60 như các đoạn README cũ. Speed truyền vào đây là **v_actual−v_ref**.
- REC cho góc tối đa ít nhất50°, speed1,2m/s, sai lệch tối đa1m, timeout4s; quiet300ms và về ngân sách gốc mới thoát. Có đường `BalanceRecovery_Reserve` bắt overspeed gần0,78m/s khi reference>0,55m/s.
- Khi REC tạm inhibit, chuyển động chờ/giảm reference rồi **có thể tiếp tục cùng lần giữ nếu lệnh vẫn mới và người dùng vẫn giữ**. Không lưu một lần nhấn đã nhả để chạy sau. Khi kẹt thật/lease hết/cancel thì phải bỏ mục tiêu cũ và xử lý `release_required`; không đánh đồng pause và cancel.
- Tiến/lùi mặc định±0,50m/s, preset nhanh±0,72m/s. Gia tốc0,30m/s², trên0,50 giảm0,15; thả hãm0,60; đảo chiều hãm0,45. Nhả bỏ đích cũ, hãm và giữ mốc mới; không khóa bánh hoặc cắt motor tức thì.
- Quay tại chỗ±1,2rad/s thường / ±2,2 nhanh; chéo±0,80/0,55rad/s trước governor theo speed/góc/bám/tâm. Hướng chéo là đường cong, không đi ngang. Lùi trái/phải đặt tên theo quỹ đạo lùi. Giữ quay liên tục, không hoàn tất một góc lưu nợ sau thả.
- Tracking budget RT8 **0,20m**, bằng chứng lead kẹt **0,30m**; nhánh đứng yên mà lệch>0,06m còn giữ. Kẹt quay có bộ đếm riêng; braking/cancel có giới hạn1m/4s.
- Governor tâm quay RT8: `max(.40,min(1,(.22−abs(x_ref−x))/.10))`. **Đây không phải chờ tới22cm mới giảm**: toàn tốc khi lệch≤12cm, giảm dần tới sàn40% ở khoảng18cm. Dùng công thức, đừng tin diễn giải điểm gãy trong tài liệu.
- Va mạnh fault8192 nếu gyro>900°/s, hoặc gyro>250°/s kèm norm accel>2g/<0,4g (code nhận norm bình phương); các giám sát khác vẫn có thể cắt motor. Không bảo đảm mọi cú xô/va tường đều phục hồi.

### Mất web và kết nối — khác hẳn các bản cũ

**RT7/RT8 không cắt motor chỉ vì đóng/ẩn/mất trang điều khiển.** Lệnh chạy hết lease350ms thì hãm và cân bằng. Gateway tự gửi PING mỗi400ms khi còn thấy STM32 balancing mới; STM32 watchdog hiện **10 giây**, không còn1 giây. ESP32 boot không tự gửi STOP. Reconnect có thể nhận lại phiên đang chạy qua `controlAdoptIfFree`, giữ epoch và sequence; không được phát lại nút giữ cũ. STOP rõ ràng và các bảo vệ cứng vẫn cắt motor. Mất nguồn STM32/ESP32 hoặc Wi-Fi lỗi không phải cùng một loại lỗi.

Web gửi MOVE8 ngay khi nhấn/nhả/đổi hướng và refresh40ms; PING200ms nếu socket không nghẽn; status200ms, bảng chi tiết500ms. Tránh xếp hàng lệnh khi `bufferedAmount>0`, nhưng neutral mới cần được gửi. HTTP/WS chạy trên loop gateway; UART capture có task riêng/mutex, archive task riêng. Tách task không loại hết ảnh hưởng ghi Flash/cache tới độ trễ. UART parser giới hạn64byte mỗi lượt; chỉ mask TIM4 quanh giải mã/cập nhật gói, không khóa toàn bộ ngắt khi quét/khôi phục DMA.

## 6. RT8 sửa gì và bằng chứng tới đâu

Ba phản hồi gần nhất: qua gồ ghề bị ngắt nhận lệnh để phục hồi; lùi chậm hơn tiến; xoay trái chậm hơn phải. `Docs/Ke_Hoach_Sua_Loi_RT8.md` là tài liệu thay đổi gần nhất.

Log27 là RT7, 7.585 mẫu/151,68s. Dữ liệu replay còn tại `Simulation/data/raw/twip_balance_R12_27_replay.csv`. Không giả định mọi file cùng số trong Downloads và các folder là cùng một lượt: kiểm header build và timestamp.

| Vấn đề | Thay đổi RT8 |
|---|---|
| Gyro gai ở pitch nhỏ gây REC | trigger gyro60→150°/s |
| Lùi reference bị tracking limiter bóp | budget0,10→0,20m, lead kẹt0,18→0,30m |
| Quay trái bị tâm quay giới hạn nhiều hơn | governor (.16,20%)→(.22,40%) theo công thức mục5 |

Không thay gain R12, observer, đặc tính motor, hướng dây hoặc gain bù encoder. Replay/synthetic tests qua, không phải mô phỏng khép kín với trạng thái mới hay thử xe mới. Dữ liệu cố định sau thay điều khiển không dự đoán được mọi quỹ đạo vật lý mới. Tài liệu RT8 từng ghi 4 lần REC cũ; phép replay giám sát hiện hành in **legacy3, RT8=0**. Không gộp chúng thành cùng một phép đo; phải nêu khác biệt phạm vi/trạng thái khởi tạo nếu phân tích lại.

**Chưa có xác nhận RT8 trên xe trong ngữ cảnh này.** Nếu người dùng nạp và gửi log mới, đó là bước cần làm tiếp; không nói ba bug đã triệt để hết ngoài thực tế.

## 7. Bản đồ file để sửa đúng chỗ

| Công việc | File |
|---|---|
| Vòng cân bằng, FSM, ARM/STOP, telemetry | `Xe_Can_Bang/Core/Src/robot_fsm.c` |
| Cấu hình build, cảm biến, PPR, interlock | `Xe_Can_Bang/Core/Inc/balance_profile.h` |
| Dựng / phục hồi | `balance_startup.h`, `balance_recovery.h` trong Core/Inc |
| Tham chiếu tiến/lùi, lease, braking, kẹt | `Core/Inc/balance_motion.h` |
| Quay/chéo/governor/giữ tâm | `Core/Inc/balance_turn.h` |
| Luật bám mục tiêu 8 hướng | `Core/Inc/phase2_motion_control.h` |
| Luật nền, observer, phân bổ motor | `Core/Inc/phase1_catalog_control.h`, `phase1_catalog_params.h` |
| UART parser DMA / scheduler | `Core/Src/esp32_comm.c`, `main.c` |
| IMU, góc và encoder | `Core/Src/bmi160.c`, `filter.c`, `encoder.c` |
| Pin/timer/I2C thực | `main.h`, `gpio.c`, `tim.c`, `i2c.c`, `usart.c`, `stm32f4xx_hal_msp.c`, file `.ioc` |
| Web UI nút/status, phiên điều khiển | `ESP32_S3_Gateway/dashboard_html.h`, `control_session.h` |
| Gateway setup, UART capture, heartbeat | `ESP32_S3_Gateway/ESP32_S3_Gateway.ino` |
| ARM/pulse/status/log RAM | `ESP32_S3_Gateway/balance_web.h` |
| Log Flash/reset/chẩn đoán | `balance_archive.h`, `balance_journal.h`, `network_health.h`, `network_routes.h` |
| Hiệu chuẩn và bài test | `measurement_web.h`, `index_html.h`, `wheel_measurement.h`, `wheel_tests_html.h`, `job_log_store.h`; STM32 `hardware_test.h`/FSM |

Các đường `Core/...` thuộc `Xe_Can_Bang`. Giữ `balance_wire.h` và `drive8_protocol.h` tương thích hai MCU. Khi đổi build ID phải cập nhật cả các chỗ match `$BINFO` trên gateway/UI/các trang test, không chỉ đổi dòng version.

## 8. Build, nạp và kiểm thử

### STM32

Mở `Xe_Can_Bang/` bằng STM32CubeIDE, build project rồi nạp ST-LINK. Giữ `.project`, `.cproject`, `.ioc`, linker scripts, `.settings`, launch, Drivers và Core. Cortex-M4F hard float (`-mfpu=fpv4-sp-d16 -mfloat-abi=hard`), có `_printf_float`.

`tools/build_stm32.ps1` là helper theo laptop (`C:/ST/STM32CubeIDE_2.1.1/...`). Nó build application và link với cấu trúc Debug đã có, **không phải bộ build sạch độc lập cho máy mới**; nếu Debug/HAL object chưa có, dùng CubeIDE build toàn project trước. Script chỉ build, không nạp. Bản nạp lưu `.build/balance_release_r12_rt8_20261024/`; RT7 còn làm dự phòng. Không suy ra binary hiện có luôn khớp source nếu vừa sửa mà chưa build.

### ESP32

Arduino IDE mở `ESP32_S3_Gateway/ESP32_S3_Gateway.ino`. Board ESP32S3 Dev Module, CDCOnBoot Enabled, Flash16MB, partition `app3M_fat9M_16MB`, OPI PSRAM. Đã dùng ESP32 core3.3.11 và WebSockets2.7.2 (Markus Sattler), thư viện laptop `D:/Arduino/libraries`. PSRAM bắt buộc cho log. Không đổi partition/Erase All Flash tùy tiện nếu cần giữ log FFat.

Arduino CLI laptop: `C:/Users/DONG NHIEN/AppData/Local/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe`. FQBN đã dùng: `esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi`. Nếu build CLI, ghi output vào `.build` và chỉ giữ bản cuối cần dùng; không sinh hàng loạt cache theo phiên bản nữa.

### Test trên laptop (không tác động motor)

Node.js và host GCC/G++ 64bit; các CJS hiện trỏ `C:/msys64/ucrt64/bin/gcc.exe`/`g++.exe`. Chạy từ gốc project:

```powershell
node tools/test_drive8_controller.cjs
node tools/test_drive8_dashboard.cjs
node tools/test_drive8_gateway.cjs
node tools/test_drive8_session.cjs
node tools/test_balance_recovery.cjs
node tools/test_net1_routes.cjs
node tools/test_rt8_tracking.cjs
node tools/test_rt8_replay.cjs
node tools/test_rt8_ab.cjs
```

Giữ `tools/test_hardware_boot.cjs` vì test controller đọc mock từ file đó; giữ `tools/lib/legacy_headers.cjs` cho A/B; giữ hai header `Simulation/experiments/motor_datasheet/phase1_catalog_*.h` cho đối chiếu luật nền. Chúng không phải rác dù nằm trong experiments. `tools/test_uart_schedule.py` kiểm logic UART; `test_balance_journal.cpp` kiểm journal. Không sửa assertion để hợp thức hóa regression.

Các kiểm thử RT8 tracking/replay, controller/UI đã chạy lại thành công sau dọn 25/10; thư viện mô phỏng cũng build được. Không ghi mọi test ở trên là vừa chạy trong phiên viết HANDOFF nếu chưa thực sự chạy.

## 9. Mô phỏng và giới hạn của nó

Điểm vào: `robot_can_bang_pid_1.sce` → `Simulation/current/review.sce`. Chuẩn bị:

```powershell
./Simulation/build_native.ps1 -Compiler C:/msys64/ucrt64/bin/gcc.exe
```

Sau đó Execute file gốc trong Scilab2026.1. Headless: `Simulation/current/review_cli.sce` (sẽ quit tiến trình khi xong). CLI laptop `C:/Program Files/scilab-2026.1.0/bin/WScilex-cli.exe`. Không nạp firmware khi mô phỏng.

- `Simulation/current/`: plant/control/observer, wrapper C dùng header STM32 thực, bộ12bài tổng quan (cân bằng, tiến/lùi, đổi chiều, thả trước tường giả lập, quay và bốn chéo).
- `Simulation/model/robot_can_bang_pid_1.sce`: hình học/động lực và khai báo nền; `config/vehicle_baseline.sce`: snapshot từ phép đo; `motor_pair_r8_20261011.sce`: đặc tính riêng từng motor; `motor_supply.sce`: VM11,7V tham chiếu.
- `build_native.ps1` build DLL local rồi alias `motion_rt5.dll`/`recovery_rt5.dll`. Tên alias RT5 là di sản; DLL biên dịch dùng header hiện có (RT8 nếu vừa build), không chứng minh nó là firmware RT5. Rebuild khi đổi header; tránh phiên Scilab giữ thư viện cũ. Không tự xóa tiến trình Scilab của người dùng.
- `scripts/run_team.sce` là đối chiếu PID nền cũ, không phải kết quả R12 hiện hành.
- Native ABI: motion state28/out29, control input12; recovery input10/out11 (state7). Nếu thêm field supervisor phải cập nhật wrapper lẫn Scilab và kiểm đồng bộ.
- Các giả định lực có tải, ma sát, tau, yaw/roll/slip và va chạm chưa nhận dạng đầy đủ. Chiều ngã không đủ để fit CG. Không tăng lực/gỡ trễ/giảm nhiễu để ép mục tiêu đạt.
- Lượt RT5 trước đây:12bài25s, bài thường pitch max≈7,45°, speed≈0,766m/s; bài va tường có pitch≈31,13°, **không đạt an toàn va chạm**. Đây là lịch sử, không được gắn lại thành chứng nhận RT8. Kết quả CSV/ảnh cũ đã xóa.
- `current/speed_limits_cli.sce` chuẩn bị±0,72/0,80/0,85/0,90m/s. Lượt trước lỗi JVM/bộ nhớ; chưa có bằng chứng để mở firmware nhanh hơn. Nếu chạy lại phải kiểm achieved reference, cờ REC/cancel/fault và dừng, không chỉ góc cuối.
- Motor danh nghĩa: π×0,065×280/60≈0,953m/s không tải; 215rpm có tải≈0,732m/s. Không dùng tốc độ không tải để bảo đảm chạy nhanh vẫn đủ lực cân bằng. Người dùng muốn xe đua nhưng không chấp nhận đánh đổi thành mất thăng bằng.

## 10. Log, chẩn đoán và cách dùng dữ liệu mới

Giữ nguyên các folder log người dùng: `test dưới sàn/`, `test kê cao/`, `56 bài test/`, `bài test 2-7/`, `12 bài test động cơ và encoder/`. **Tên folder không luôn phản ánh điều kiện thật**: có lần log trong `test kê cao` thực ra bánh trên sàn. Phải xác nhận điều kiện của từng lượt, không áp một lời xác nhận cho mọi log tương lai.

`Simulation/data/raw/` và `processed/` còn giữ dữ liệu hiệu chuẩn/đối chiếu. Có CSV replay27, nhưng không mặc định original TXT27 ở đây. Nếu cần original mà không tìm thấy hãy xin người dùng cung cấp; không tự coi CSV đã chứa toàn bộ header/stop diagnostics.

### Giao thức và trường dữ liệu

- `$MOVE8,direction,turn,epoch,sequence,preset*`, direction/turn∈{−1,0,1}, preset∈{0,1}. Epoch do phiên ARM xác nhận, sequence tăng; không để PING làm mới lease di chuyển. `$MOVE` cũ không phải giao thức8hướng. `$CMD`, `$PROFILE`, `$ARM,7` không bật lại controller cũ.
- Binary balance122byte, sync0x60a5, CRC16-CCITT initFFFF; little-endian float. Log decimate50Hz từ control200Hz, thêm2s tail sau dừng.
- CSV40cột, đọc tên header từ `balance_web.h`, đừng dùng parser27/35cột cũ. Row index0based: state4, fault6, PWM7/8, ticks9/10, raw gyro11..13, accel14..16, pitch17, gyro18, vL/R19/20, x21, control_v22, control_theta23, base24, integral25, dropped26, controller27, control_phase28, distance_limit29, x_ref30, v_ref31, epoch32, motion_flags33, direction34, psi_ref35, w_ref36, turn37, acknowledged sequence38/rx_ms39.
- `control_phase`:0thường,1dựng,2REC. `motion_flags` bitmask:1ready,2moving,4braking,8tracking_limited,16release_required,32has_moved,64cancelled,128blockage,256fast,512yaw_governor,1024linear_braking,2048yaw_rate_control.
- Fault bitmask:1STOP,2heartbeat,4tilt,8distance,16speed,32I2C,64timing,128gyro_calibration,256IMU_config,512nonfinite,1024fixture_motion,2048startup_timeout,4096recovery_timeout,8192impact. Kiểm đúng code/context; đừng nhầm `flags` của hardware-test với balance fault.
- `dt_us` và `execution_us` phân biệt nhịp vòng/chi phí tính. Target5000µs; spacing guard4–6ms. ACK web gồm chu kỳ telemetry/HTTP, không phải latency một chiều Wi-Fi.

RAM balance9000frame≈180s hoạt động cộng tail, đầy thì ngừng ghi, **không tự ghi đè/không tự dừng xe**. Tải sauSTOP và chờ2s; lưu từng lượt trước xóa. Nhiều ARM có thể trong một file: phải tách theo state/seq/epoch; timestamp/sequence có thể reset khi STM32 khởi động lại.

FFat archive:4file tối đa, không tự xóa/ghi đè; batch tối đa50frame, CRC; chỉ tự format khi phân vùng trống0xFF. Là best-effort: mất nguồn có thể mất đuôi hoặc hỏng FAT. Endpoint `/balance-log`, `/balance-status`, `/balance-archives`, `/balance-archive?name=...`, `/network-status`, `/network-log`; `/firmware-info` xác định trang đang chạy. Còn12bài motor và56bài hiệu chuẩn qua `/wheel-tests`, `/calibration`.

Nếu Wi-Fi biến mất rồi trở lại và RAM log về0: kiểm boot_id/reset_reason, power/brownout/panic/watchdog; không kết luận chỉ là iPhone đổi5G. Người dùng từng gặp đúng biểu hiện này. Đo5V/3V3 ngay tại ESP32 khi motor khởi động/đảo chiều; điện áp tĩnh tại XL4015 chưa loại được sụt ngắn. Không tắt brownout/watchdog để che lỗi. `tools/Tai_Log_Cuu_Ho.*` còn để hỗ trợ tải log.

Với log mới: xác định build/VM/điều kiện thả tay → tách lượt → tìm **sự kiện đầu tiên** trước ngã/dừng → đối chiếu lệnh, ACK, lease, gyro/pitch, v_ref/v_actual, x_ref−x, w_ref/yaw, limiter/PWM/fault/GSTOP → tái hiện bug bằng test/replay → nếu sửa lực/quỹ đạo thì cần mô phỏng khép kín và thử thật tiếp. Không gán mọi lỗi xoay cho encoder hoặc mọi ngã cho CG.

## 11. Những tài liệu còn cũ hoặc khẳng định quá mức

- `README.md`: đầu đúng RT8/NET5, nhưng URL có vRT5, còn câu rời tab dừng motor và đoạn ngưỡng60°/s của RT4. **Không dùng chúng làm hành vi mới nhất.**
- `ESP32_S3_Gateway/README.md`: đầu RT8, phần dưới kế thừa RT4 NET1; chỉ STM32 RT4 thì nạp ESP là hướng dẫn lịch sử, không áp cho cặp RT8/NET5.
- `Docs/Phat_Trien_Tiep.md`, `Thong_So_Xe_Hien_Tai.md`: còn BASE60/MOVE1/SPEED1/SPEED2,40/60cm/s, PING1s, log104byte/35cột100Hz và đường dẫn tài liệu/script đã bị dọn. Dùng phần cơ khí có đối chiếu, không dùng mô tả giao thức/phiên bản cũ.
- `Docs/Phan_Cung_Va_Ket_Noi.md`: có PID cascade, encoder937, giới hạn góc cũ, tầng A4950/cọc cũ. Pinout phải đối chiếu `.ioc` và driver.
- `Simulation/README.md`: tiêu đề RT5/aliasRT5 là di sản wrapper; firmware nguồn hiệnRT8. Cần chạy lại mô phỏng sau thay header; build thành công không phải chứng minh xe ổn.
- `Docs/Ke_Hoach_Sua_Loi_RT8.md`: gần hiện tại nhất nhưng còn cách giải thích governor chưa đúng điểm bắt đầu giảm và câu suy luận motor/roughness chắc chắn từ một log. HANDOFF nêu rõ công thức và giới hạn bằng chứng.

Lượt viết bàn giao không sửa các tài liệu trên để tránh phát sinh thay đổi ngoài yêu cầu. Phiên sau có thể đồng bộ tài liệu trong một công việc riêng, giữ tính lịch sử rõ ràng.

## 12. Dọn dẹp đã thực hiện và cách không tạo rối trở lại

Đợt đầu đã xóa khoảng23,15GiB đầu ra build/thử nghiệm. Đợt25/10 dọn thêm script RT1, cấu hình R5–R10/S45–S50 không được gọi, DLL RT1–RT4, IDE log theo ngày, file kiểm tra dọn và thư mục rỗng. Không có thống kê byte chính xác riêng đợt sau; không nói đã xóa thêm23GB. Một số nguồn thử cũ từng track trong Git hiện hiển thị D; raw log/hiệu chuẩn/core không bị xóa.

Còn giữ: firmware RT8, cấu hình IDE/build, source mô phỏng hiện tại, raw/processed, datasheet và thông số thiết kế, test hiện hành/phụ thuộc, hai bản nạp RT8 vàRT7. Giữ `.metadata`/`.vscode`/Debug/Drivers vì có cấu hình/khả năng nạp cần thiết; không thấy tên hidden/build là xóa cả.

Tạo đầu ra mới dưới `.build/` hoặc `Simulation/results/`/`reports/`, không rải script một lần và báo cáo ngoài root. Dọn các kết quả không còn dùng sau kiểm chứng; giữ **mã test tái lập có ý nghĩa và dữ liệu gốc**. Xác minh đường dẫn tuyệt đối trong DA_HTN trước xóa đệ quy, kiểm dependency/hash nếu cần, không xóa log người dùng chỉ vì nó có chữ test.

## 13. Quy trình tiếp tục đề nghị

1. Đọc HANDOFF, xem build ID và Git diff hiện tại; không bắt đầu từ commitRT4 trên main rồi tưởng đó là bản laptop.
2. Nếu yêu cầu xử lý ba bug: trước tiên hỏi/kiểm tra log RT8 mới, vì bản RT8 đã có sửa và chưa có xác nhận thực tế; không chồng một lần nới ngưỡng nữa chỉ từ mô tả RT7.
3. Giữ những chức năng người dùng đã đánh giá tốt: cân bằng R12, dựng60°, chống xoay, 8hướng, giữ–thả, phục hồi và log. Thay có mục tiêu, test giao thoa: đảo chiều, quay rồi thả, qua xóc trong lúc giữ, loss/reconnect và STOP.
4. Nếu tối ưu realtime/I2C DMA: đo execution/jitter/age trước; thêm timeout/stale/error recovery và cập nhật mô hình trễ, không chỉ thay HAL API. Nhánh `pipeline` của đồng đội từng được yêu cầu review, không tự merge.
5. Chỉ tăng tốc khi kiểm đủ mô-men/tải/VM, dự trữ lực phanh và góc, tracking/cancel và tốc độ từng bánh. Không hứa không ngã với mọi lực xô/địa hình.
6. Build/test sau thay đổi; xác nhận đúng source/build ID của cả hai board và trang nhúng. Người dùng tự nạp bằng CubeIDE/Arduino, thử trên xe rồi gửi TXT/JSON cho vòng tiếp theo.

Người dùng ưu tiên làm kỹ hơn làm nhanh, giải thích rõ và hành động trong phạm vi đã yêu cầu. Không xin lại xác nhận cho thao tác thông thường đã được cho phép; nhưng không tự nạp motor, push hoặc xóa dữ liệu ngoài mục tiêu của yêu cầu hiện tại.

## 14. Dấu nhận diện nguồn tại lúc bàn giao

SHA-256 bên dưới giúp nhận ra tài liệu đã cũ nếu nguồn tiếp tục đổi; không thay thế build/test và không chứng nhận binary trên xe.

| File | SHA-256 |
|---|---|
| `Xe_Can_Bang/Core/Inc/balance_profile.h` | `9e50db9e8d1015db6ecea72e110db1ec54b5cc8167d2de467acdeacf6f8dcd63` |
| `Xe_Can_Bang/Core/Inc/balance_motion.h` | `3077099f48989d63f31d783886d04770a0c5b99e7ac2c7c274a854534c2b9894` |
| `Xe_Can_Bang/Core/Inc/balance_turn.h` | `93f0240606f01e16c741d746281de73536986bbeb98da2a70edfc339d7de999c` |
| `Xe_Can_Bang/Core/Inc/balance_recovery.h` | `1b938a43a2d7f1bfd7ea61140b5b45a5b0cb6e53a7f855a4f4ce1dfd2a961fc4` |
| `ESP32_S3_Gateway/firmware_build.h` | `e27be223562b711fa3d8345f13b2d2b79ea4c8352ef720d72e1c34b61a16c532` |
| `ESP32_S3_Gateway/control_session.h` | `4d72c07ade983452c8b3ac0754291ea4cfd6d1083226c7ecca5e01de15273375` |
| `ESP32_S3_Gateway/dashboard_html.h` | `fc4308a0c271d497bab9fcc1e1f02074819b7538256ee4dcb0fc8f95e884d447` |
