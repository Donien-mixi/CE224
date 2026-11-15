> **Trạng thái đồ án · 15/11/2026.** Nền R12 đã được thử cân bằng và dựng từ 60°. Giai đoạn điều khiển web đã có 8 hướng và log. Workspace đang ở **RT29-NOCUT / NET30** (8 hướng, preset 50/72/80, interlock có trễ, giữ mốc vị trí khi bị đẩy, không còn tự ngắt oan `fault 4096`), đã build, kiểm thử trên máy và push Git; chưa có log xe cho riêng RT29. Mô hình tính pin kê thêm 10 mm. PCB local một mặt đồng đã qua kiểm tra KiCad với 31 dây cầu. Xem [tổng quan và các việc còn mở](Docs/Tong_Quan_Do_An.md).

> **DỌN DẸP VÀ PHÁT HÀNH 15/11/2026 — trạng thái repo để nộp.** `.build/` (1,8 GB gồm 30+ bản release cũ, thư mục chạy thử, phụ thuộc KiCad) đã xoá; **chỉ bản đang dùng được giữ trong repo** tại `Firmware/RT29_NOCUT_20261115/` kèm `SHA256.json` (hash binary) và `SOURCE_SHA256.json` (hash 12 tệp nguồn đã sinh ra đúng cặp binary này — đã kiểm lại 12/12 khớp). Log xe thô, `output/RTxx/` (script + metrics từng vòng), `Simulation/data/`, `Simulation/reports/` đã **chuyển ra ngoài repo** (`D:\DA_HTN_archive_20261115\`), không xoá mất; riêng `Simulation/data/raw/twip_balance_R12_27_replay.csv` (721 kB) được giữ lại vì `tools/test_rt8_replay.cjs` cần. Đã xoá phần sinh tự động: `Simulation/results/` (2,9 GB), `.metadata/`, `Xe_Can_Bang/Debug/` (28,6 MB), `ESP32_S3_Gateway/build/`, `tmp/`. Sáu test của bản RT8/RT9 và khối đo tốc độ bằng tay (đã gỡ khỏi firmware) chuyển sang `tools/legacy/` kèm bảng test tương đương ⇒ **30/30 host test pass** (trước là 30 pass + 6 fail cũ). 36/36 ca mô phỏng native đủ 20/20 s. Thêm `Docs/Bang_Ma_Loi_Tu_Dong_Dung.md` liệt kê mọi đường tự cắt motor. Build ID **không đổi**, không sửa một dòng luật điều khiển nào trong lượt này.

> **HIỆN HÀNH RT29-NOCUT / NET30, 15/11/2026 — bỏ ngắt oan 4096 sau 4 s.** Log RT28 (12)(3) (7 345 mẫu / 158,4 s, 3 lần ARM): xe **tự ngắt 3 lần**, tất cả **fault 4096**, tại t=**11,52 / 85,50 / 156,48 s** — mỗi lần cách một lần **vào REC1 đúng 4,00 s** (7,52 / 81,50 / 152,48 s). Nguồn là `BalanceMotion_Fault` (phanh không hoàn tất trong 4 s), không phải timeout của REC1: phanh chỉ "xong" khi |v|<0,06 m/s suốt 200 ms, mà khi bị đẩy liên tục thì không bao giờ xong. Ngắt oan — ngay trước mỗi lần cắt xe **dựng thẳng** (pitch −0,22 / +1,12 / −0,86°, gyro ≤15 °/s, v 0,11–0,22 m/s) và mốc chỉ lệch 8–19 cm. RT29: `if(s->braking&&s->hold_anchor)return 0;` — phanh **do nhiễu** (vào REC1) không có hạn 4 s; **giữ** hạn 4 s/1,25 m cho phanh nhả ga có lệnh (RT19), giữ biên 1,25 m cho cả hai, giữ timeout 4 s của REC1. `test_rt29_nocut`: phanh nhiễu 12 s không fault; phanh nhả ga vẫn fault 4096 đúng 5,0 s; lệch 1,30 m vẫn fault 8. **30/30 host test pass**; `test_build_id_sync` pass. Artifact `Firmware/RT29_NOCUT_20261115/` (`STM32_BAL_R12_RT29_NOCUT.bin` `16effa54…`, `ESP32_RT29_NOCUT_NET30.merged.bin` `0581361f…`), web `?v=RT29-NOCUT-NET30`. **Đã push Git 15/11/2026; chưa có log xe cho RT29.** Xem [RT29](Docs/RT29_Tu_Ngat_4096.md).

> **BẢN TRƯỚC RT28-ANCHOR / NET29, 15/11/2026 — sửa mốc vị trí khi bị đẩy.** Log RT27 (10)(2)/(11)(2): sau khi chạy các hướng rồi dừng, **đẩy từ trước xe không hồi đúng vị trí, đẩy từ sau thì hồi** (cân bằng vẫn giữ). Nguyên nhân: khi REC1 bắt đầu, BalanceTurn gọi BalanceMotion_BeginBrake(s,position,now) và hàm này **gán x_ref := vị trí hiện tại** ⇒ mốc cũ bị xoá và thay bằng chỗ xe đang bị đẩy lệch: log (10)(2) t=54,36 nhảy **+0,219 → +0,110**, log (11)(2) t=38,02 nhảy **−0,967 → −0,943** (1 tick). Đẩy từ sau làm thân ngả nhanh nên REC1 bật sớm (lệch vài cm → trông như hồi đúng), đẩy từ trước bật muộn (lệch 10–11 cm → mất vị trí). RT28: thêm hold_anchor + BalanceMotion_BeginBrakeHold (phanh **không** chụp lại mốc) dùng ở chỗ REC1 bắt đầu; khi cờ bật thì **không** theo vị trí trong lúc phanh và **không** chụp lại mốc ở cửa sổ RT19; cờ xoá khi có lệnh lái mới/kết thúc phanh. Lệnh lái mới và nhả ga có lệnh **vẫn** chụp mốc tại chỗ của nó (giữ hành vi RT19).  test_rt28_anchor: mốc giữ **0,0000** suốt cú đẩy 28 cm + REC1; cú gọi cũ trên bản sao đổi 0,500→−0,280 (chứng minh test bắt đúng lỗi); lệnh mới 0,500; dừng có lệnh 0,300. **29/29 host test pass**;  test_build_id_sync pass. Artifact .build/balance_release_r12_rt28_anchor_20261115/ (STM32_BAL_R12_RT28_ANCHOR.bin dc1f1410…, ESP32_RT28_ANCHOR_NET29.merged.bin e956f61d…), web ?v=RT29-NOCUT-NET30. **Chưa nạp xe, chưa push Git.** Log 7/8/9 (đẩy ngay sau ARM) không đổi vì lúc đó used=false. Xem [RT28](Docs/RT28_Giu_Moc_Vi_Tri_Khi_Bi_Day.md).

> **BẢN TRƯỚC RT27-RECOVER / NET28, 15/11/2026 — gỡ cơ chế catch/return, trả push về nền RT21.** Ba log mới (1)(3) 12,4 s, (36) 16,1 s, (2)(3) 5,2 s, build RT26-PUSH+NET27, **không có lệnh lái**. Kết quả: (1)(3) và (2)(3) **fault 4** ở pitch −53°, (36) **fault 8** (cắt theo quãng) khi thân mới **+13,8°**. Số đo tại lúc cắt: _ref = **−0,87/−0,70/−0,82 m/s** — đúng bằng vận tốc thật, vì catch đặt _ref := vận tốc nên số hạng Kv·(v−v_ref) **bằng 0**, luật mất phanh theo vận tốc và tiếp tục *đẩy* xe theo hướng đang lăn. Phát lại cửa sổ trước cú ngã: **tham chiếu RT21 phanh mạnh hơn catch tới 5,3 N** (ở v −0,75: −6,95 N so với −2,37 N). Lỗi thứ hai: REC1 bị tôi siết còn **+0,35 m** → fault 8 oan ở |x−x_ref|=0,378 m. RT27: **gỡ hoàn toàn** alance_catch.h/alance_push.h (không còn phase=3, không tham chiếu bám thân, không drive-home), **REC1 trở lại RT21** (50°, 1,2 m/s, +0,8 m), giữ hai sửa đổi chỉ để tránh cắt motor oan: interlock **200 ms** (RT23) và REC1 **giữ primed** sau khi thoát.  est_rt27_push phát lại 39 mẫu thật: max|v_ref| = 0,000 khi không có lệnh, lực phanh tới −9,57 N, và test fail nếu ai đặt lại _ref := v. **27/27 host test**; 12 ca mô phỏng 20/20 s. Artifact .build/balance_release_r12_rt27_recover_20261115/ (STM32_BAL_R12_RT27_RECOVER.bin d1c3889…, ESP32_RT27_RECOVER_NET28.merged.bin 6c6c9a4b…), web ?v=RT29-NOCUT-NET30. **Chưa nạp xe, chưa push Git.** Giới hạn: đẩy làm thân vượt ~50° hoặc xe tới 0,9 m/s với thân ngả ngược vẫn ngã (phản điện động chỉ cho ~1,5 N kéo tới). Xem [RT27](Docs/RT27_Tro_Ve_Nen_RT21_Chong_Chiu.md).

> **SỬA SAU KHI NẠP 15/11: build ID lệch NGÀY.** Người dùng nạp RT27 và **không bật/không chọn được ĐO LẠI GYRO trên web**. Nguyên nhân: BALANCE_BUILD đã lên BAL-20261115-… nhưng 9 literal trong gateway còn BAL-20261114-… (dashboard_html.h 2, alance_web.h 4, control_session.h 2,  est_net21_link.cjs 1) → matched() false, canDrive() false, /operate trả 409 với rm, banner báo sai phiên bản. Đã sửa hết 9 literal và **thêm  ools/test_build_id_sync.cjs** (đòi mọi literal BAL-\\d{8}-R12-8DIR-… trong gateway/tools trùng BALANCE_BUILD, có probe lệch ngày phải fail). **28/28 host test pass.** Chỉ cần nạp lại **ESP32**: ESP32_RT27_RECOVER_NET28.app.bin = ec817195…, merged = 2b4dbe0…; STM32 không đổi d1c3889…. Lưu ý: nút ĐO LẠI GYRO chỉ mở khi motor đã DỪNG và phải mở mục Công cụ chi tiết.

> **BẢN TRƯỚC RT26-PUSH / NET27 — sửa lỗi trôi xe của RT25.** Hai log mới (5)(3) 1 493 mẫu/29,8 s và (6)(1) 1 436 mẫu/28,7 s, build RT25-CATCH+NET26, **không có lệnh lái**, không fault. Người dùng báo: huých nhẹ mà xe **trôi rất xa** (tổng quãng 1,28 m và **2,51 m**) và có lúc ngã. Nguyên nhân: alance_catch.h (RT25) đặt **x_ref := vị trí thân** khi catch, nên chỉ **33 và 11 mẫu** catch đã kéo mốc vị trí **3,66 m / 4,56 m** — xe mất mốc cũ, mỗi cú đẩy kết thúc ở chỗ mới; thêm vào đó ngưỡng catch quá thấp (60 °/s×20 ms) và REC1 cho đi +0,8 m. RT26: alance_push.h thay alance_catch.h — CATCH **chụp và khôi phục x_ref mỗi tick** (chỉ thả _ref), ngưỡng **80 °/s×20 ms hoặc lean 12°**, trần góc 40°, control_phase=3; **RETURN** đặt _ref = 1,6·(x_ref−x) giới hạn **±0,30 m/s**, deadband **4 cm** khi không giữ lệnh và xe 
eady; REC1 chỉ còn **+0,35 m**. Logic nằm trong BalanceMotion8_Update để **mô phỏng native cũng chạy**.  est_rt26_push phát lại đúng 26 mẫu cú ngã RT24: catch bật 1 lần, lực **−4,59 N** so với **+0,98 N** (Δ−5,57 N), phanh đỉnh −8,88 N, **x_ref không đổi**; probe tắt catch fail. **27/27 host test**; 24 ca mô phỏng 20/20 s. Lưu ý trung thực: bộ --disturbance (xô 1–4 N) **đã ngã 9/12 ca ngay cả khi tắt chính sách** nên không dùng làm bằng chứng chống chịu. Artifact .build/balance_release_r12_rt26_push_20261114/ (STM32_BAL_R12_RT26_PUSH.bin 898a7642…, ESP32_RT26_PUSH_NET27.merged.bin 6579e074…), web ?v=RT29-NOCUT-NET30. **Chưa nạp xe, chưa push Git.** Xem [RT26](Docs/RT26_Giu_Vi_Tri_Khi_Bi_Day.md).

> **BẢN TRƯỚC RT25-CATCH / NET26, 14/11/2026 — thêm khả năng chống chịu khi bị đẩy.** Hai log mới  wip_balance_R12 (3)(3) (24 s, **ngã 1 lần**) và (4)(3) (53 s, không ngã), không có lệnh lái, build RT23+NET24. Ngã ở t=22,02 s: ault 4 khi pitch **−25,5°**, PWM 0/0, sau khi đã bị bão hoà 2499. Ba nguyên nhân phần mềm: **(a)** tham chiếu làm luật đẩy xe vào cú ngã — tại t=21,88 s (pitch −4,25°, gyro −116 °/s, v 0,88) các số hạng Kx·(x−x_ref)=+1,1, **Kv·(v−v_ref)=+5,4**, Kθ·θ=−1,1, Kω·ω=−2,4 → **+1,5 N vẫn đẩy TỚI** trong khi thân đã ngả về sau; cho hai tham chiếu bám thân thì cùng trạng thái là **−4,6 N phanh**, cả cú đẩy tới **−9,57 N**. **(b)** REC1 thoát ở 21,42 s và cần 0,5 s yên để nạp lại primed; cú đẩy đến sau 0,14 s nên phase=0 suốt → chỉ còn trần 25°. **(c)** Trần 25° cắt motor (PWM 0/0) đúng lúc motor là thứ duy nhất còn cứu được. RT25: alance_catch.h mới — **|gyro pitch| ≥ 60 °/s trong 20 ms** hoặc **|lean| ≥ 10°** thì vào **CATCH** (control_phase=3): x_ref:=vị trí, _ref:=vận tốc, **bỏ yêu cầu bẻ hướng**, trần góc nâng **40°**; thoát khi yên 150 ms rồi ramp phanh dừng tại chỗ mới; khoá catch trong pha dựng xe. REC1 **giữ primed sau khi thoát** để cú đẩy kế tiếp nạp lại ngay. Không đổi gain/observer/allocator R12, luật chuyển động/bẻ hướng, governor, REC1 limit, khung 122 B. Kiểm chứng:  est_rt25_catch **phát lại đúng cửa sổ ngã** (26 mẫu thật) → catch bật ở pitch −4,25°/gyro −116 °/s và cho −4,59 N ngay, đỉnh −9,57 N, trong khi luật RT24 vẫn +1,53 N (Δ−6,12 N); probe tắt catch **fail**; **27/27 host test**; **36 ca mô phỏng trùng từng số RT24** — nhưng **36 ca đó KHÔNG phủ catch** (catch nằm ở 
obot_fsm.c, harness Scilab không biên dịch). Artifact .build/balance_release_r12_rt25_catch_20261114/ (STM32_BAL_R12_RT25_CATCH.bin sha256 89248532…, ESP32_RT25_CATCH_NET26.merged.bin 61e00fe…). Web http://192.168.4.1/ui/r12?v=RT29-NOCUT-NET30. **Chưa nạp xe, chưa push Git.** Giới hạn: ở 0,93–0,95 m/s sức phản điện động chỉ cho ~1,5 N kéo tới; cú đẩy vượt quá khả năng lực/ma sát vẫn ngã. Xem [RT25](Docs/RT25_Chong_Chiu_Khi_Bi_Day.md).

> **BẢN TRƯỚC RT24-LEANREF / NET25, 14/11/2026 — sửa từ log RT23 trên xe twip_balance_R12 (1).txt.** Người dùng báo **quay trái vẫn chậm hơn quay phải** sau RT23. Log 3 277 mẫu (65,5 s, RT23+NET24, CRC/gaps/reset 0): quay phải đạt **2,40 rad/s phẳng**, quay trái **2,18 median, p10 1,65, 58 % số mẫu dưới 2,38**; pitch quay trái **+6,54°** (p90 7,87) so với **−2,19°** khi quay phải; trôi tâm **16,9 cm so với 3,9 cm**. Nguyên nhân phần mềm còn lại: taper nghiêng vẫn dùng **pitch tuyệt đối** (12−|pitch|)/6 với knee 6°, mà cú xoay tại chỗ 2,40 rad/s của xe luôn nghiêng 6–8° ⇒ (12−6,5)/6=0,92 → 2,21 và (12−7,9)/6=0,69 → 1,66, khớp đúng số đo. RT24: taper dùng **|pitch − tư thế trung tính|** (giá trị học được +1,074°, qualified=1), **toàn quyền tới 8° vượt trung tính**, giảm 8→12°, 0 khi vượt 12°. Phần **cơ khí** thì vẫn còn: cùng tốc độ quay, bánh trái luôn chạy nhanh hơn (0,286 vs 0,261 m/s) và cần duty lớn hơn ⇒ lực dọc không triệt tiêu hết, thân phải ngả để bù; ngay cả khi w_ref=2,40 phẳng, yaw trái vẫn 2,285 so với 2,378 (−3,9 %). Kiểm chứng:  est_rt24_sym **phát lại chính trace pitch của xe** (4 tick/mẫu) → RT24 trái 2,40/min 2,40, phải 2,40/min 2,40; luật RT23 (probe) cho trái **1,95 median / 1,37 min** nên probe fail đúng như log. 26/26 host test pass; 36 ca mô phỏng 20/20 s không fault (pitch y hệt, yaw ca 6/12 tăng 12°, ca 9/10 tăng 8°). Artifact .build/balance_release_r12_rt24_sym_20261114/ (STM32_BAL_R12_RT24_SYM.bin sha256 dd379204…, ESP32_RT24_SYM_NET25.merged.bin df3c1d5f…). Web http://192.168.4.1/ui/r12?v=RT29-NOCUT-NET30. **Chưa nạp xe, chưa push Git.** Việc kiểm phần cứng: đổi chỗ hai motor rồi đo lại 8 s mỗi bên — nếu độ lệch theo motor thì do motor/hộp giảm tốc, nếu không thì do khung/bánh/ổ trục; firmware không đo dòng nên chưa có số dòng từng motor. Xem [RT24](Docs/RT24_Taper_Nghieng_Quay_Trai.md).

> **BẢN TRƯỚC RT23-SYM2 / NET24, 14/11/2026 — sửa từ 8 log RT22 của người dùng.** Ba lỗi tìm được, cùng một bộ log 8 tệp (toàn bộ `BAL-…-RT22-SYM`/`NET23`, `boot=384`, `reset=power_on`, `archive_saved=0`, `archive_error=1`, CRC/gaps/reset = 0). **(1) Quay trái mất tốc độ, quay phải không:** quay phải đạt đúng 1,30/2,40/2,40 rad/s ở cả ba mốc, quay trái chỉ còn 0,88/0,98 ở 50, 0,65–0,88 ở 72, **0,62–0,73 ở 80**. Nguyên nhân là taper "tâm quay trôi" `(0,22−drift)/0,10`: cú xoay tại chỗ làm thân ngả **+4,8…+5,9° khi quay trái** (phải chỉ −2,6…−4,2°) nên trục bò 2–3 cm/s và trôi tới **15–17 cm**, vượt knee 12 cm → ghìm còn 74 % (trôi 0,146 m ⇒ 2,40×0,74 = 1,78 rad/s, đúng số đo). **(2) Preset 72 chéo mất 40–60 %:** `wheel_cap` lấy **vận tốc đo** so với **trần trục 0,80** thay vì trần bánh, nên ở 0,72–0,79 m/s cap còn 0,10–0,46 rad/s và nhảy theo nhiễu; log cho 0,51–0,69 rad/s so với 1,15 yêu cầu (flag512 63–100 %), trong khi cùng cú đó ở 80 đạt 1,15. **(3) `fault 16` cắt cả hai motor khi trục vượt trần một mẫu → xe ngã:** cả hai log 72 ngã ở đúng tick nhả lệnh quay; yaw hãm làm trục 0,74 → 0,86 m/s, `BalanceStop(16)` đặt PWM 0/0 khi thân mới **1,8–2,0°**, rồi thân đổ **0 → 105° trong 0,24 s**; ảnh chụp chạy thẳng cũng là `fault 16`, PWM 0/0, pitch −13,63°. Sửa: trần bánh theo preset (50/72/80 = 0,80/0,98/1,05 m/s mỗi bánh), cap tính trên **v_ref** + 10 cm/s dung sai; taper tâm knee **22 → 45 cm**, sàn **40 → 55 %**; interlock mới `BalanceInterlock` — trần `max(preset, |v_ref|+0,12)`, ngắt khi vượt **200 ms** liên tục, ngắt ngay chỉ khi **>1,45 m/s**. Không đổi gain/observer/allocator R12, governor RT22, REC1, trần ngang 0,9, taper nghiêng, blockage, khung 122 B. Kiểm chứng: **25/25 host test** (thêm `test_rt23_interlock`, mở rộng `test_rt22_turn`, cả hai tự fail với hằng số RT22), **36 ca mô phỏng** 20/20 s không fault, pitch y hệt RT22 từng ca; ca 6 chéo ở 80: yaw 4–14 s là 0,92/0,75/0,12/0,10/0,10 (RT22) → **1,06/1,08/1,13/1,10/1,12** (RT23). Artifact `.build/balance_release_r12_rt29_nocut_20261115/` (ESP32 đã build lại với build-ID đúng) (`STM32_BAL_R12_RT23_SYM.bin` sha256 `3000e363…`, `ESP32_RT23_SYM_NET24.merged.bin` `6cb6822e…`). Web `http://192.168.4.1/ui/r12?v=RT29-NOCUT-NET30`. **Chưa nạp xe, chưa push Git.** Còn mở: nguyên nhân ESP32 reset `watchdog ngắt` chưa chứng minh (NET24 chỉ giới hạn dò vùng trống còn 64 kB và đưa `/archive-format` sang tác vụ lưu), trôi khi xoay còn 15–17 cm (cơ khí), và **3 log chạy thẳng 50/72/80 đã mất vì `archive_error=1` cần ghi lại**. Xem [RT23](Docs/RT23_Nut_That_Va_Loi_Toc_Do.md).

> **BẢN TRƯỚC RT22-SYM / NET23, 14/11/2026 — sửa từ log RT21 `(1)`.** Người dùng báo xe **lùi nhanh/nhạy hơn tiến**. Log RT21 (7 779 mẫu, 2 ARM, CRC/gaps/reset = 0) xác nhận: cùng lệnh 80 cm/s, `v_ref` đạt 95 % mất **2,26–4,56 s khi tiến** nhưng **0,82–1,94 s khi lùi**; governor ghìm **15–100 %** số mẫu khi tiến so với **1–10 %** khi lùi; có lần `v_ref` kẹt 0,68 trong **4,78 s**. Nguyên nhân: quy tắc nghiêng dùng ngưỡng **tuyệt đối 5°** trong khi tư thế trung tính của xe là **+1,5°** và lúc tăng tốc thân nghiêng thật **+5,8…+8,7°**, cộng với việc **đóng băng `v_ref`** mà chỉ mở khi `|pitch|<3°`, `|gyro|<15`, `|v−vref|<0,08` — xe đang chạy không bao giờ đạt cả ba, nên khựng 0,5–4,8 s. RT22: ngưỡng nghiêng **9°/11°** (khởi hành), mở **5°/30 °/s**, **bò 0,06 m/s² thay vì đóng băng**, học tư thế trung tính nhanh hơn (0,5 s + 60 mẫu) và **giữ qua STOP/ARM**; bẻ hướng: quay tại chỗ **1,3/2,4 rad/s**, chéo **0,90/1,15 rad/s**, gia tốc yaw **4,5 rad/s²**, trần ngang **0,9 m/s²** (cũ 0,4 nên mọi cú chéo ở 0,8 m/s bị ép còn 0,50 rad/s). Không đổi duty>0,90, quy tắc trễ, R12 gain/observer/allocator, encoder, REC1, lease/STOP, khung 122 B. Kiểm chứng: `test_rt22_sym` (tiến/lùi đều 2,21 s, đúng 0,5000/0,7200/0,8000, không đóng băng, quá tải vẫn bị ghìm) và `test_rt22_turn` (2,40 rad/s sau 0,6 s; 1,12 rad/s chéo; trần bánh/nghiêng/kẹt giữ nguyên) — **cả hai tự chứng minh độ nhạy bằng cách fail với hằng số RT21**. 24/24 host test đang dùng pass; 36 lượt mô phỏng 20,000/20 s không fault (edges 12 ca: 6 ca đầu trùng RT21, ca 7–12 khác vì bẻ hướng nhanh hơn). Artifact `.build/balance_release_r12_rt29_nocut_20261115/` (ESP32 đã build lại với build-ID đúng). Web `http://192.168.4.1/ui/r12?v=RT29-NOCUT-NET30`. **Chưa nạp xe, chưa push Git.** Xem [RT22](Docs/RT22_Doi_Xung_Va_Be_Huong.md).

> **BẢN TRƯỚC RT21-ANTISTALL / NET21, 13/11/2026.** Hiệu chuẩn gyro lúc cấp nguồn giữ nguyên **500 mẫu × 10,3 ms ≈ 5,15 s** (bản thử 200 mẫu đã khôi phục; hash trùng bản 13/11 `a1509a52…`). Bản NET21 (AP lên trước log, liveness 3 s + backoff, `hb` 1 Hz, phục hồi vùng log Flash, `boot_id/reset_reason`) là nền của RT22. Artifact `.build/balance_release_r12_rt21_antistall_net21_20261113/`. Web `http://192.168.4.1/ui/r12?v=RT21-NET21`. Xem [NET21](Docs/NET21_Ket_Noi_Ben_Vung.md), [QUICKSTART](Docs/RT21_QUICKSTART_Hieu_Chuan_Nhanh.md) (đã đánh dấu KHÔNG nạp).

> **ĐÃ GỠ 14/11/2026: kênh chẩn đoán USB (NET22).** Đã dựng thử một kênh stream 40 cột qua cáp USB/UART0 kèm `tools/live_monitor.ps1`; người dùng quyết định **chỉ cần gửi log**, nên toàn bộ đã gỡ (`debug_console.h`, 3 công cụ, tài liệu, artifact NET22) và firmware trở lại NET21 đúng từng byte. Không còn cổng debug trên gateway; muốn xem dữ liệu thì dùng log RAM/Flash tải từ trang web như cũ.

> **THÍ NGHIỆM ĐÃ KHÔI PHỤC, 14/11/2026.** Từng thử rút hiệu chuẩn gyro lúc boot xuống 200 mẫu (2,05 s thay vì 5,15 s) với lập luận sai số bias ~0,008 °/s so với ~0,005 °/s; frame đầu ~2,2 s. **Không dùng bản này** — đã trả về 500 mẫu. Số đo và lập luận giữ ở [QUICKSTART](Docs/RT21_QUICKSTART_Hieu_Chuan_Nhanh.md).

> **BẢN TRƯỚC RT21-ANTISTALL / NET20, 11/11/2026.** Sửa từ ba log RT20 (80 cm/s): `v_ref` dưới 0,60 trong 51–64 % thời gian giữ tiến, cờ governor bật 48–65 %, có deadlock 1,64 s ở 0,12 m/s. Governor nay chỉ coi trễ bám tốc là thiếu dự trữ khi tham chiếu đang giữ/phanh (ngưỡng 0,12 → 0,30 khi tăng tốc) và cho tham chiếu kẹt dưới 0,15 nhảy tới 0,15 để phá ma sát tĩnh. Không đổi duty>0,90, dự báo nghiêng>5°, retreat, gain/observer/phân bổ lực R12, encoder, REC1. Đo trên log: 0,79 m/s dùng 82–83 % duty; mô phỏng 0,90 và 0,95 cho cùng quỹ đạo (trần ~0,89 m/s) nên **giữ preset 80**. 20/20 host test (thêm `test_rt21_antistall`), 36/36 ca mô phỏng. Build cả hai; artifact `.build/balance_release_r12_rt21_antistall_20261111`. Web `http://192.168.4.1/ui/r12?v=RT21-NET20`. **Chưa nạp xe, chưa push Git.** Còn mở: dự báo nghiêng vẫn là nguồn đóng băng chính (28–32 %), chưa có log RT21, ba log RT20 có ESP32 brownout. Xem [RT21](Docs/RT21_Chong_Ket_Governor.md).

> BẢN TRƯỚC RT20-POSTREC / NET19, 08/11/2026: trần sau REC1 có thời hạn 1,5 s; preset 95 → 80 cm/s; 19 host test, 36 ca; artifact `.build/balance_release_r12_rt20_postrec_20261108`.

> BẢN TRƯỚC RT19-HOLD80 / NET18, 08/11/2026: sửa từ log `(13) (2)` — mốc hướng sau quay giới hạn 0,6 s, chốt lại mốc vị trí sau phanh, preset 95 → 80 cm/s; 18 host test, 36 ca mô phỏng; artifact `.build/balance_release_r12_rt19_hold80_20261108`.

> LỊCH SỬ RT18-LAUNCH / NET17, 03/11/2026: đã sửa sau log RT17(12)(2), chưa có log xe thật RT18. First motion-ready sau dựng dùng500ms yên và neutral, không đòi trở về trong8cm mốc ARM; không dời mốc khi idle, thao tác mới chốt mốc như trước. Stuck vẫn error>6cm/speed<5cm/s; evidence300ms nếu abs(vref)>=.30, ngược lại1s nếu dir*vref>=.15, còn lại1.5s. Lead>30cm tối đa100ms; tiny-ref bị ghìm vẫn hủy hữu hạn. Giữ nguyên từng byte governor RT17 (thử nới inverse-phase đã loại vì weakVM10/12), gains/encoder/signs/IMU/yaw/REC1/ARM60/8hướng/50-72-95/lease/STOP. Replay7đoạn hủy: RT17 7, RT18 0; đoạn đứng lệch11cm cổng RT18 mở~.56s, đây là replay50Hz không phải chuyển động mới. Mô phỏng cuối36/36ca20s thường/biên/launch50 qua và có kiểm tra nhận hướng; thử thêm95+mốc lệch lớn1/2, ca+3deg vẫn ngã nhưRT17. Không cam kết mọi95/mọi va chạm. 11host tests qua, build STM+ESP, artifact .build/balance_release_r12_rt18_launch_20261103. Web http://192.168.4.1/ui/r12?v=RT18-NET17. Test50tiến/lùi trên sàn trống trước. Xem Docs/RT18_Khoi_Hanh_Va_Chong_Ket_Nham.md và output/RT18/. Không nạp bằng tool, không push Git. Các ghi chú sau là lịch sử.

> NGUỒN LỖI RT17 — log (12)(2), 03/11: người dùng xác nhận sàn trống, không chạm xe. 4 lượt, 7 lần tiến bị gán kẹt sai trong lượt cuối. Lệnh+1 tới STM, vref dương nhưng inverse response/đảo chiều làm bánh còn lùi; governor lag>0.12 ghìm tham chiếu khi PWM còn nhỏ, sai lệch vị trí>6cm + speed<5cm/s đếm300ms rồi hủy. Lượt gần58s không mở motion-ready do xref0, xe nghỉ lệch−8..−14cm và điều kiện readiness<8cm. Driver encoder qua test wrap/sign/LPF; ticks/velocity/position nhất quán, chưa có bằng chứng cần đảo encoder. Không sửa firmware trong phiên chẩn đoán, không tạo RT18. RT17 hiện **chưa đạt yêu cầu điều khiển ổn định**, không nạp lại cùng bản để kỳ vọng hết lỗi. Chi tiết Docs/RT17_Log12_Chan_Doan_Lenh_Tien.md; output/RT17_Log12/. Cần kiểm chứng sửa phối hợp governor/khởi hành/phát hiện kẹt/readiness, giữ gains và dấu/wiring.

> LỊCH SỬ 03/11: STM **RT17-SYNC** + ESP **NET16**, đã có log xe thật RT17 (12)(2), có lỗi khởi hành/nhận lệnh cần sửa. Xem banner chẩn đoán mới phía trên. Đã đọc hai log RT16 (10)(1)/(11)(1), tổng5lượt, nhận ra gate tăng tốc bị lệch do mốc đứng yên và dwell readiness lặp sau hủy. RT17 học mốc predictor từ neutral tự cân bằng (±2°, không đổi CF/gain), giữ ngưỡng RT16; bỏ gia tốc lưu sai hướng khi đảo; abort200ms xác nhận xong không chờ500ms lần hai. REC1 luôn bắt đầu quỹ đạo phanh dù còn giữ nút; lệnh mới khi thân ổn chạy≤25cm/s/quay0.4rad/s, sau REC cùng lượt giữ≤50cm/s/quay0.8; thao tác mới ở chế độ thường trả mức chọn. Flag32768 ghi giới hạn này. Xung250dps+norm bất thường yêu cầu REC1; >900dps vẫn cắt, các giới hạn REC50°/1m/4s giữ nguyên; reserve quá tốc sớm. Hiệu chuẩn gyro lỗi không ghi đè bias tốt. R12 gains/observer/allocator/PPR/đấu dây giữ nguyên. 24/24ca thường/biên qua, nhưng ca xô nhỏ11/12 và thử thách1–4N chỉ4/12 (RT16 3/12): **không tuyên bố chống mọi va chạm hoặc đã cân tốc hai chiều thực tế**. Build cảMCU, chưa nạp bằng tool. Xem Docs/RT17_Tien_Lui_Va_PhuC_Hoi.md và output/RT17/. Root mô phỏng dùng đúng wrapper hiện hành; model đã bỏ giả định tự nhả nút mỗi lầnREC1, ghi header snapshot trước build. Các banner dưới đây là lịch sử firmware.

> LỊCH SỬ RT16-STABLE / NET15: sửa abort timer, phanh góc yên6°, phanh riêng1.25m/4s, vị trí giám sát từ tick, BMI kiểm tra cấu hình/BUSY/retry/bus clear. Hai log mới xác nhận không còn32/4096/8, vẫn kẹt nhận lệnh và tiến chậm. Xem Docs/RT16_Phan_Tich_7_Log_Va_On_Dinh.md.

> LỊCH SỬ RT15: STM RT15-FAST + ESP NET14. Tăng cap tham chiếu72/95 từ0,35→0,50m/s² tới50cm/s,0,18→0,22 phía trên;50giữ nguyên. Gain R12 giữ nguyên; bản này đã có log lỗi. Xem Docs/RT15_Tang_Toc_Va_Realtime.md. Giữ dây trái đỏ→AOUT2,trắng→AOUT1.

> LỊCH SỬ WEB: RT14-RELEASE-NET13; đã gỡ bảng quay tay encoder và đặt mốc, giữ ghi log encoder và điều khiển. STM RT14 không đổi.

> LỊCH SỬ FIRMWARE: STM RT14-RELEASE + ESP NET12-ENC; sửa bộ tạo vận tốc khi thả vềneutral để không tăng |vref|, không đổi gain R12. Xem Docs/RT14_Log5_Va_Dau_Day.md. Motor trái đỏ→AOUT2,trắng→AOUT1; PCB/schematic/W1-W2 đã đồng bộ, ERC/DRC/parity0.

> CẤU HÌNH ĐẤU DÂY ĐÃ XÁC NHẬN 03/11/2026: **motor trái đỏ(M1) → AOUT2, trắng(M2) → AOUT1**; motor phải đỏ → BOUT1, trắng → BOUT2. Encoder và PA8/PA9 không đảo. Bảng màu trái ngược trong ghi chép lịch sử không dùng để lắp lại xe.

> PCB HIỆN HÀNH 02/11/2026: đã chuyển TWIP_Mainboard sang B.Cu-only kèm33dây cầu cách điện mặt hàn; không có track/plane F.Cu, không có via. THT pad vẫn *.Cu theo KiCad, chỉ gia công B.Cu. Giữ nguyên kích thước/chân/vị trí linh kiện và4lỗM3. Xem Hardware/TWIP_Mainboard/SINGLE_LAYER.md, SINGLE_LAYER_WIRES.csv và SINGLE_LAYER_PREVIEW.html. Archive_2Layer_20261102 giữ bản hai lớp. ERC/DRC/parity=0 khi jumper đã mô hình; chưa phê duyệt sản xuất hoặc xác nhận nhiễu/tải. Firmware không thay trong lượt này.

# Bàn giao đồ án xe hai bánh tự cân bằng

Cập nhật bàn giao: **30/10/2026**, múi giờ Asia/Saigon. Nội dung kỹ thuật nền được lập ngày25/10, kiểm tra lại các điểm nhận diện nguồn ngày30/10. Workspace: `D:\DA_HTN`.
Tài liệu được đối chiếu với mã đang có trên laptop, không chỉ tóm tắt lời trao đổi. Các đường dẫn bên dưới tương đối với thư mục gốc project để có thể chuyển máy.

## 0. Bắt đầu phiên mới tại đây — 30/10/2026

### Ưu tiên đọc: cập nhật 01/11/2026 — R12 RT13 RESP95 / NET10

Nguồn mới nhất là `BAL-20261101-R12-8DIR-RT13-RESP95` và `TWIP-WEB-20261101-BAL-R12-8DIR-RT13-RESP95-NET10`. Đọc `Docs/RT13_RESP95_Log34_35.md` trước các mục lịch sử dưới đây. Hai log34/35 thực tế là RT12 GOV95, không mất mẫu/reset STM32, vref dừng khoảng87–88,5cm/s, chưa giữ95; PWM vùng nhanh đã quanh90%. Người dùng yêu cầu lấy11,7V làm điều kiện tham chiếu, không có VM đo live. RT13 chỉ đổi hồi dự phòng lúc khởi hành72/MAX dưới0,5m/s: predictor hold6° thay5°, recovered dwell100ms thay300ms cùng điều kiện gyro/pitch/PWM; giữ dốc0,35/0,18, jerk, bộ cân bằng R12, dựng60°, yaw, lease và bảo vệ tốc độ cao. Không ép95 bằng gỡ giới hạn.

Đối chiếu Scilab cũ/mới cùng plant/seed ở11,7V, 12ca20s/bản, đủ thời gian và không fault/REC; ca50 dữ liệu giống hệt. Khởi hành đạt50 nhanh hơn, không phải mọi mốc tốc độ đều nhanh hơn; MAXlùi đạt80 còn chậm hơn khoảng0,155s trong ca danh định. Chưa thử RT13 trên xe, không bảo đảm95 hoặc vận hành lâu. Bản nạp `.build/balance_release_r12_rt13_resp95_20261101/`, URL `/ui/r12?v=rt13-resp95-net10`; nạp lại cả hai MCU do web kiểm build ID. Giữ RT12 dự phòng. Dùng `node tools/run_governor_sim.cjs --stress` cho bài governor; runner lưu adapter/header/lịch, các launcher cũ không mặc nhiên có governor. Các lượt12V chỉ là sàng lọc, không gán thành kết quả11,7V. Không thay cơ khí/motor plant để ép khớp tốc độ thực. Chưa push/nạp xe trong phiên này.

### RT12 GOV95/NET9 — sửa REC nhầm72 và quản lý giới hạn motor

Source hiện hành **RT12-GOV95/NET9**, giữpreset50/72/95, innerR12frozen không đổi. Log31/32 có38lầnREC ở72 với thân góc nhỏ;33ảnh tái hiện predictor cũ, gate mới0/38. MAX ngã ba lần khi cảPWM2499 trước lỗi8192. Đã thêm reference shaper200Hz bằng `balance_governor.h`: jerk, feedbackPWM/IMU/encoder, giữ/giảm nhẹ vref khi thiếu dự phòng, capxref lead10cm không che kẹt bánh. SửaReserve gọi khi thân có dấu hiệu đáng lo; không hạ preset95 nhưng không bảo đảm actual95 khi hết motor authority. Bit16384=governorlimited. Thêm latch lỗiBMI để không đọc lại bus lỗi trong mỗiIRQ; CALIB dừng mới thửBMI_Init ngoàiIRQ, không autoARM. Đọc `Docs/RT12_GOV95_Phan_Tich_31_33.md` và các nguồn nghiên cứu trong đó. 7bộtest +6ca mô hình chẩn đoán qua; chưa thử thực tếRT12/chưapush. Bản nạp riêng `balance_release_r12_rt12_gov95_20261031`, không dùng thư mục build mặc định cũ. Không nhận đây làMPC/RG có chứng minh hay bảo đảm chạy95/chạy lâu trênbreadboard.

### RT11 MAX95/NET8 — thử mạnh hơn và giảm trễ theo log29/30

Người dùng yêu cầu chủ động thử mạnh, không lấy mô phỏng làm chuẩn duyệt. Source **RT11-MAX95/NET8**:50/72/MAX95, dốc tuyến tính0,60/0,30, hãm0,90, đảo1,0; quay3,0/hãm8rad/s², reset yaw integral khi đảo dấu. MAX envelope1,12m/s/từng bánh1,20, predictor1,05; governorMAX nền1,05. Giữ bảo vệ cứng. Web chuyển primary pointer không chèn neutral, gia hạnMOVE20ms/poll100ms. RAMlog30000mẫu10phút do hai log cũ đầy9000 và thiếu6926/357mẫu. Tuổi lệnh median23ms/P95 44ms không phải latency toàn tuyến; log29 đã có saturation/fault8192/16, nên không gọi95 là đã chứng minh ổn định. Đọc `Docs/RT11_MAX95_Realtime_20261031.md`. Bản nạp tách riêng `balance_release_r12_rt11_max95_20261031`, chưa nạp/thửRT11, chưapush. RT10 còn làm dự phòng.

### RT10 MAX80 — bản vận hành sau ba lượt thử31/10

Source mới **RT10-MAX80 / MAX80-NET7**, thayLOAD2. Đọc `Docs/RT10_MAX80_Phan_Tich_Log_Va_Ban_Van_Hanh.md`. Ba lượt thực tế đạt đỉnh88,6–90cm/s nhưng đều chạm dự phòngPWM; chọn vận hành50/72/MAX80. Filetest(2) chứa lại lượt(1), đã tách epoch1/4/7. Bỏ bộ thử12bài/advancedcalibration/PULSE vàLOADTEST khỏi đường chạy; giữ gyro cơ bản, log/health/archive. MAX có envelope0,95m/s và predictor0,90m/s, giữ khi hãm để không cắt nhầm khi đổi mức. Không đổi luật/gains/motor/cơ khí R12. Bản nạp riêng `.build/balance_release_r12_rt10_max80_20261031`; chưa thửRT10 trên xe, chưa push các thay đổi.

### Sửa LOAD2 sau log thử bị từ chối —31/10/2026

Source hiện tại chuyển sang **RT9-LOAD2 / LOAD2-NET6**. Log `twip_load_speed_R12.txt` có683mẫu, không có mẫu chạy thử; từ chối reason6 vì bánh tự chỉnh khoảng5,9cm/s vượt ngưỡng5cm/s củaLOAD1. Đã nới đầu vào thành0,5giây/góc5°/gyro35°/s/từngbánh12cm/s; từ chối trước khi chạy được thử lại sau lệnh trung tính, không cầnARM mới. Lượt đã chạy vẫn không tự lặp; bảo vệ khi chạy giữ nguyên. Đọc `Docs/Sua_Dieu_Kien_Bat_Dau_LOAD2_20261031.md`, dùng bản build riêng `balance_release_r12_rt9_load2_20261031` thayLOAD1. Chưa nạp/thửLOAD2 trên xe.

### Cập nhật31/10/2026 — chế độ thử tốc độ cân bằng LOAD1

Người dùng chọn **đo tốc độ cao nhất khi vẫn tự cân bằng**, xe breadboard, nguồn khai báo12V. Đã thêm chế độ web tạmLOAD1 trên nền RT8; source hiện tại STM32 `BAL-20261031-R12-8DIR-RT9-LOAD1`, ESP32 `TWIP-WEB-20261031-BAL-R12-8DIR-RT9-LOAD1-NET6`. Chưa nạp, chưa thử xe thật, chưa commit/push các thay đổi này. Giữ commitRT8 trênmain làm mốc trước thử nghiệm.

Đọc `Docs/Thu_Toc_Do_Can_Bang_LOAD1_20261031.md`: bật cân bằng trước, đứng yên1giây, bấm bắt đầu riêng, tăng tốc thẳng tới điểm đặt0,95m/s, kết thúc tối đa8giây/6m hoặc bảo vệ. Một motor≥90%PWM liên tục100ms thì hãm để giữ quyền chỉnh cân bằng; kết quả không phải RPM cực đại tuyệt đối. Không sửa gains, khối lượng888g, luật R12, observer, hiệu chuẩn motor, tốc độ thường50/72 hoặc filePCB.

Đã qua10CJS, mô phỏng chẩn đoán kín3ca12V/10,8V/mất lệnh; chi tiết giới hạn mô hình trong tài liệu trên. Có bản build riêng `.build/balance_release_r12_rt9_load1_20261031/`; kiểm build/hash trước khi nạp. Lỗi BMI160/STM32 mất liên lạc sau ngã mạnh chưa được sửa dứt điểm bởi LOAD1. Người dùng sẽ gửi log thử để phân tích; gỡ chức năng tạm sau đợt thử theo yêu cầu tiếp theo.

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

| Mục | Trạng thái hiện hành (cập nhật 14/11/2026) |
|---|---|
| STM32 | `BAL-20261115-R12-8DIR-RT29-NOCUT` trong `Xe_Can_Bang/Core/Inc/balance_profile.h` |
| ESP32 | `TWIP-WEB-20261115-BAL-R12-8DIR-RT29-NOCUT-NET30` trong `ESP32_S3_Gateway/firmware_build.h` |
| Web | `http://192.168.4.1/ui/r12?v=RT29-NOCUT-NET30`; tham số v chống cache, không chọn luật điều khiển |
| Wi-Fi | `TWIP_RACER_S3`, mật khẩu `12345678`, AP cục bộ không có Internet |
| Git | Nhánh `main`; origin `https://github.com/Donien-mixi/CE224.git` |
| Commit hiện hành | Xem `git log -1`; `b95cd71` là mốc RT8/NET5. Toàn bộ RT9–RT21 còn uncommitted |
| Bản nạp | `Firmware/RT29_NOCUT_20261115/` (kèm `SHA256.json`, `SOURCE_SHA256.json`); đã build và push, **chưa có log xe** |

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
node tools/test_net21_archive.cjs
node tools/test_net21_link.cjs
node tools/test_rt22_sym.cjs
node tools/test_rt22_turn.cjs
node tools/test_rt23_interlock.cjs
node tools/test_rt24_sym.cjs
node tools/test_rt27_push.cjs
node tools/test_rt28_anchor.cjs
node tools/test_rt29_nocut.cjs
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
