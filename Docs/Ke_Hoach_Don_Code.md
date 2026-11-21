# Kế hoạch dọn code — RT44-TURN (giữ nguyên 100 % chức năng)

Mục tiêu: code **dễ đọc như người viết** — mỗi hàm một việc, thông số tập trung một chỗ, không còn khối
comment lịch sử dài — **mà xe vẫn chạy đúng như RT44 hiện tại**.

Ràng buộc bất di bất dịch:

- Không đổi: gain/observer/phân bổ R12 (`CAT_K`, `cat_allocate`, `CAT_VNL/FRICTION/START`), CF alpha
  0,998, zero lắp 2,16694264915°, `BALANCE_PPR` 898,225, dấu encoder, khung 122 B, `$MOVE8`/lease,
  ARM ±60°, ready 200 ms, heartbeat 10 s, các chốt an toàn (REC1, interlock, governor, trim).
- **Không bump build ID** trong suốt quá trình (đây là refactor, không phải bản mới). Nếu hex ra khác
  baseline thì phải chứng minh hành vi bằng test + mô phỏng, và chỉ khi đó mới cân nhắc bản mới.

## 0. Hiện trạng (đo được)

| Khu vực | File | Dòng | Ghi chú |
|---|---:|---:|---|
| `Xe_Can_Bang/Core/Inc` (lớp giám sát) | 32 | 3 109 | **1 131 dòng comment (36 %)**, 54 `#ifndef` probe, 94 khối `#if/#ifdef`, 258 `#define` |
| `Xe_Can_Bang/Core/Src` | 19 | 4 179 | `robot_fsm.c` 448 dòng là trung tâm |
| `ESP32_S3_Gateway` | 12 | 1 873 | `dashboard_html.h` 584 dòng (trang web nhúng) |
| `tools` | 64 | 6 275 | 42 host test + script |
| `Docs` | 26 | 3 374 | báo cáo theo vòng |
| Rác sinh ra | — | — | `.build` **665 MB**, `Simulation/results` **1 110 MB**, `Xe_Can_Bang/Debug` 29 MB, `.metadata` 7 MB |

Vấn đề “đọc không hiểu”: (1) comment kể lịch sử (“log RT36 (2)(5) cho thấy…”) nằm trong code;
(2) thông số rải khắp 7 header; (3) nhánh chỉ dùng cho test/probe xen giữa logic; (4) thư mục sinh ra
chiếm 1,8 GB mà không nói rõ cái nào xoá được.

## 1. Cổng kiểm chứng cho MỌI bước (không thoả thì không đi tiếp)

```
baseline: hex RT44 = 0ee37a5c…  ·  42/42 host test  ·  sim 12/12 @80, 12/12 edge, 12/12 launch
```

- Mỗi bước: chạy lại **42 host test**; nếu chạm quỹ đạo/tốc độ thì chạy **3 bộ mô phỏng**.
- Bước chỉ đổi tên/comment/di chuyển `#define`: **hex phải giống baseline từng byte**. Đây là bằng
  chứng mạnh nhất “không đổi chức năng”.
- Bước có đổi cấu trúc code: hex có thể khác → phải có bảng đối chiếu hằng số (trước/sau) + test xanh.
- Không commit khi cây đỏ; mỗi bước một commit nhỏ để `git revert` được.

## 2. Các bước

### Bước 1 — Một file thông số duy nhất (`tuning.h`)

- Tạo `Xe_Can_Bang/Core/Inc/tuning.h`: **toàn bộ** thông số chỉnh được, nhóm theo chức năng — tốc độ
  50/80/90, cua (rate/trần bánh/trần ngang/taper), REC1, governor, interlock, trim bánh trái, UART,
  lease. Mỗi dòng: `TEN  giá_trị  // đơn vị — để làm gì`.
- Chuyển 258 `#define` từ `balance_*.h` vào đây, giữ nguyên tên + `#ifndef` (test override được).
- Kết quả: “chỉnh thông số” chỉ cần mở **một** file.

### Bước 2 — Comment: bỏ lịch sử, giữ lý do

- Xoá toàn bộ đoạn kể log (“log RT36 (2)(5)…”, “RT19 HOLD80…”) khỏi code; mỗi hằng/hàm còn **1–3 dòng**
  nói *để làm gì* và *đánh đổi gì*. Bằng chứng lịch sử đã có trong `Docs/` và git.
- Mục tiêu: comment của `balance_*.h` từ **1 131 → dưới 400 dòng**, không còn chữ “RT..” trong code.

### Bước 3 — Bỏ nhánh chết / gom probe

- 94 khối `#if/#ifdef`: phân loại (a) dùng cho probe của test, (b) chết trong firmware. Loại (b) xoá;
  loại (a) gom về **một khối duy nhất ở đầu mỗi module**, có tên rõ (`TWIP_PROBE_*`).
- Sau bước này code chỉ còn đường chạy thật + một cửa probe cho test.

### Bước 4 — Hàm có tên, tham số rõ

- `balance_motion.h` (340) và `balance_turn.h` (325): tách thành các hàm nhỏ có tên nghĩa
  (`Motion_RampToTarget`, `Motion_BeginBrake`, `Turn_LimitByWheelEnvelope`, …), mỗi hàm một việc, tham
  số là đầu vào rõ ràng thay vì biến rời trong một hàm 200 dòng.
- Đầu mỗi hàm: 1 dòng mô tả + 1 dòng “gọi khi nào”.
- `robot_fsm.c`: vòng 200 Hz giữ nguyên thứ tự 16 bước nhưng mỗi bước gọi một hàm có tên; thêm bảng
  “thứ tự pipeline” ở đầu file.

### Bước 5 — Dọn thư mục / file sinh ra

- Xoá được bất cứ lúc nào (đều sinh lại được): `.build/` (665 MB), `Simulation/results/` (1,1 GB, giữ
  lại 1–2 lần chạy gần nhất), `Xe_Can_Bang/Debug/` (29 MB), `.metadata/` (7 MB).
- Thêm `Docs/Cay_Thu_Muc.md` (1 trang): mỗi thư mục là gì, mở file nào bằng IDE, file nào là **sinh ra**
  và xoá được — để bạn không phải đoán.
- Kết quả: repo chỉ còn source + firmware + tài liệu.

### Bước 6 (tuỳ chọn) — Tách `.c` cho lớp giám sát

- Chuyển `balance_motion/turn/recovery` từ header-inline sang `.c` + `.h` (như code người viết bình
  thường). **Đây là bước rủi ro nhất** vì phải thêm file vào project IDE (`Debug/` makefile) và sửa
  `motion_native.c` của mô phỏng. Chỉ làm nếu bạn muốn, và sau khi bước 1–5 đã xanh.
- Nếu làm: phải build lại và **nạp lại cả hai board** (hex đổi), nên sẽ bump RT45.

## 3. Thứ tự thực hiện & thời lượng

| Bước | Nội dung | Rủi ro | Hex đổi? | Cần bạn làm gì |
|---|---|---|---|---|
| 1 | `tuning.h` | thấp | không (nếu chỉ di chuyển `#define`) | — |
| 2 | Comment | rất thấp | không | — |
| 3 | Bỏ nhánh chết | trung bình | **có thể** | — |
| 4 | Tách hàm | trung bình | **có thể** | — |
| 5 | Dọn thư mục | không | không | — |
| 6 | Tách `.c` (tuỳ chọn) | cao | có | nạp lại 2 board |

Sau mỗi bước tôi báo: hex giống/khác baseline, test, mô phỏng, và diff tóm tắt.

## 4. Hai điều cần bạn quyết

1. **Có làm bước 6 (tách `.c`) không?** Làm thì code “giống người” nhất nhưng phải nạp lại 2 board;
   không làm thì vẫn đạt mục tiêu dễ đọc nhờ bước 1–4.
2. **Cho xoá 1,8 GB thư mục sinh ra** (`.build`, `Simulation/results`, `Xe_Can_Bang/Debug`,
   `.metadata`) không? Đều sinh lại được, nhưng lần build đầu sẽ chậm hơn và mất các lần chạy mô phỏng
   cũ (kết quả đã ghi trong `Docs/`).

## 5. Tiến độ (cập nhật 21/11/2026)

Bước 6 (tuỳ chọn) đang làm **từng module, mỗi module một commit nhỏ**; đến 21/11 đã **xong 3/3**.
**Không bump build ID, không nạp board**: đây là refactor, hành vi phải y nguyên. Bước 5 (dọn thư mục)
đã xong ở commit `da4583d`.

| Module | Trạng thái | Commit | Bằng chứng |
|---|---|---|---|
| 1/3 `balance_recovery` | xong 21/11 | `c8c299c` | 38/38 bộ test chạy được; mô phỏng @0,80 12/12, số từng ca trùng |
| 2/3 `balance_motion` | xong 21/11 | commit 2/3 (sau `c8c299c`) | 38/38 bộ test + 9 probe; hex cùng kích thước; 3 bộ mô phỏng trùng từng byte |
| 3/3 `balance_turn` | xong 21/11 | commit 3/3 | 38/38 bộ test + 9 probe; 3 bộ mô phỏng trùng **73/73** file |

Sau 3/3, cả ba module đều là `.c` + `.h`; `balance_motion.h` 340→137 dòng, `balance_turn.h` 325→125
dòng, `balance_motion.c` 259 dòng, `balance_turn.c` 172 dòng.

### Bước 6 — module 2/3: `balance_motion`

- **Tách**: `Core/Src/balance_motion.c` chứa 8 hàm (`BeginBrake`, `BeginBrakeHold`, `Cancel`, `Fault`,
  `Impact`, `corner_budget`, `maxhead`, `maxhead_next`); `balance_motion.h` (340 → 275 dòng ở bước này;
  sau 3/3 còn 137) chỉ còn
  thông số (giữ nguyên tên + `#ifndef`), struct, `Clear`, prototype và `BalanceMotion_Update`.
- **`BalanceMotion_Update` vẫn inline ở bước này** (sau 3/3 đã chuyển nốt sang `.c`): caller chèn hook
  governor qua macro `BALANCE_MOTION_FILTER` / `BALANCE_MOTION_ANCHOR` **trước khi** include
  (`robot_fsm.c:30`, `Simulation/current/motion_native.c:4`, 10 bộ test). Việc đổi hook được xử lý ở mục 3/3.
- **Token**: 8 thân hàm đã chuyển + `BalanceMotion_Update` **trùng từng token** với bản cũ (so
  `gcc -E -P` sau khi bỏ khoảng trắng). Khác duy nhất: `static inline` → liên kết ngoài.
- **Host test**: 42 tệp `test_*.cjs`, trong đó **4 tệp rỗng 0 B** (`test_production_*`) → **38/38 bộ
  chạy được PASS**, gồm 9 probe hằng số cũ phải fail. **24 bộ test + `run_governor_sim.cjs`** được thêm
  `balance_motion.c` vào lệnh gcc. `tools/test_uart_schedule.py` PASS.
- **Build**: CubeIDE headless (workspace `D:\CubeIDE_ws_verify`), **0 lỗi 0 cảnh báo**;
  `text 83 952 / data 500 / bss 7 516` **giống hệt** bản trước khi tách. `.bin` 84 460 B,
  SHA256 `f23eb682…` (trước khi tách: `7814c318…`) — khác vì địa chỉ/độ inline đổi, không phải đổi
  hành vi. Build ID vẫn `RT44-TURN`, có trong `.bin`.
- **Mô phỏng** (so **từng file** với lần chạy ngay trước khi tách, cùng bộ ca/seed):
  `--stress` @0,80: **73/73** file trùng; `--stress --edge` @0,90: **72/73**;
  `--stress --launch --launch50` @0,90: **72/73**. File khác duy nhất là `scilab_console.txt`
  (đường dẫn/ thời điểm). Mỗi bộ **12 ca**, tất cả `20,000/20 s`, `fault = 0`.
- **Bẫy khi chạy lại**: `TWIP_SPEED_MAX` phải khớp bộ đang so (stress so ở 0,80; edge/launch ở 0,90).
  Nếu lệch, `guard*.csv` khác ở cột `speed_limit` (1,12 so với 0,90) dù quỹ đạo giống — dễ đọc nhầm
  là hồi quy.
- **Không làm**: không nạp board, không push `main`, không đổi `CAT_K`/observer/khung 122 B, không bump
  build ID.
### Bước 6 — module 3/3: `balance_turn`

- **Tách**: `Core/Src/balance_turn.c` (172 dòng) chứa `BalanceTurn_Approach`, `BalanceTurn_headroom`,
  `BalanceTurn_headroom_next` và `BalanceMotion8_Update` (thân ~180 dòng); `balance_turn.h` (325 → 125
  dòng) chỉ còn thông số + struct + `Clear` + `BalanceMotion8_Maneuver` (4 dòng) + prototype.
- **Phải đổi hook governor**: trước đây caller define macro `BALANCE_MOTION_FILTER` / `_ANCHOR` **trước khi**
  include; macro không đi qua được translation unit, nên khi `BalanceMotion8_Update` sang `.c` thì
  `balance_motion.c` không thấy hook và governor bị bỏ qua. Thay bằng **ba hàm hook yếu** trong
  `balance_motion.c` (`BalanceMotion_FilterHook`, `BalanceMotion_AnchorHook`,
  `BalanceMotion_GovernorActive`); `robot_fsm.c`, 10 bộ test và harness mô phỏng cài bản mạnh; nơi không
  cài thì giữ đúng hành vi cũ (ramp tuyến tính, không neo).
- **Lỗi bắt được nhờ test**: bản đầu chỉ có 2 hook nên `test_rt18_launch` thấy `v_ref=0` thay vì
  `1,25e-5` (assert `-sign*s.v_ref>0`). Truy nguyên: khối `#ifndef BALANCE_MOTION_FILTER` (reset `v_ref`
  về vận tốc đo khi bắt đầu lệnh mới) chạy sai nhánh trong `.c`. Thêm `BalanceMotion_GovernorActive` cho
  đúng nhánh → test xanh lại. Ghi lại vì đây là kiểu lỗi im lặng, chỉ lộ ở assert biên.
- **Token**: 4 thân hàm turn + `BalanceMotion_Update` **trùng từng token** với bản cũ (so `gcc -E -P`),
  trừ đúng hai câu lệnh hook và một câu `GovernorActive`.
- **Host test**: 38/38 PASS, gồm 9 probe. `tools/lib/legacy_headers.cjs` phải sửa: hằng số `>0.30` và
  `>0.20` đã chuyển vào `balance_motion.c` nên bản legacy giờ copy + patch ở `.build/legacy_src`
  (`test_rt8_ab`, `test_rt8_replay` so sánh đúng).
- **Build**: CubeIDE headless, 0 lỗi 0 cảnh báo; `text 84 176 / data 500 / bss 7 516` (trước bước này:
  83 952 / 500 / 7 516 — tăng 224 B do gọi hàm thay vì inline). `.bin` 84 684 B, SHA256 `3921ebd5…`;
  build ID vẫn `RT44-TURN`, có trong `.bin`.
- **Mô phỏng**: so từng file với lần chạy ngay trước khi tách — `--stress` @0,80 **73/73** file trùng,
  `--stress --edge` @0,90 **73/73**, `--stress --launch --launch50` @0,90 **73/73**; chỉ
  `scilab_console.txt` khác đường dẫn/thời điểm. 12 ca/bộ, tất cả 20,000/20 s, `fault 0`.
- **Không làm**: không bump build ID, không nạp board, không push `main`.

### Bước 1 — `tuning.h` (xong 21/11)

Mục tiêu của bước: "chỉnh thông số chỉ cần mở **một** file". Đã gom **55 macro** từ 5 tệp
(`balance_motion.h` 12, `balance_turn.h` 19, `balance_recovery.h` 8, `balance_governor.h` 11,
`balance_profile.h` 5) vào [`Core/Inc/tuning.h`](../Xe_Can_Bang/Core/Inc/tuning.h), chia 6 nhóm theo chức
năng, mỗi dòng `#define TÊN giá_trị  // đơn vị — để làm gì`.

- **Giữ nguyên** tên macro và `#ifndef` để host test / mô phỏng còn override bằng `-D` (9 probe đang dùng).
- `tuning.h` được include từ `balance_motion.h`, `balance_turn.h`, `balance_recovery.h`,
  `balance_governor.h`, `balance_profile.h` nên mọi đường biên dịch đều thấy đủ.
- **Ba macro dùng biến của module gọi** (`BALANCE_MOTION_SPEED`, `BALANCE_MOTION_ACCEL`,
  `BALANCE_TURN_PURE_RATE`/`DIAGONAL_RATE`) vẫn là macro, có chú thích rõ "dùng biến `s`/`old`", vì đổi
  sang hàm sẽ làm mất khả năng override `-D` của probe cũ.
- `balance_profile.h` chỉ còn **thứ không được chỉnh bừa**: build ID, luật R12, hiệu chuẩn cảm biến,
  cổng ARM, capture, chốt góc/quãng, heartbeat; comment lịch sử RT23–RT29 đã bỏ khỏi tệp này.
- **Bằng chứng "không đổi hành vi"**: `38/38` host test PASS; build CubeIDE 0 lỗi 0 cảnh báo;
  `.bin` **giống từng byte** bản trước khi gom (`text 84 176 / data 500 / bss 7 516`, SHA256
  `3921ebd5…`). Chỉ di chuyển macro thì mã máy phải y nguyên — đây là cổng mạnh nhất của bước 1.
- **Không làm**: không bump build ID, không nạp board, không push.

### Bước 2 + 3 — comment và nhánh probe (xong 21/11)

#### Bước 2 — bỏ lịch sử, giữ lý do

- **31 nhãn "RTxx"** trong comment code đã được viết lại thành 1–3 dòng nêu *để làm gì / đánh đổi gì*:
  `robot_fsm.c` (9 khối), `balance_governor.h` (7 khối), `balance_interlock.h`, `balance_trim.h`,
  `bmi160.c`, `usart.c`, `tuning.h`.
- Còn đúng **1 chỗ chứa "RT44"**: literal build ID `BALANCE_BUILD "BAL-20261120-R12-8DIR-RT44-TURN"` —
  phải giữ (đổi là lệch build ID với ESP32 và phá `test_build_id_sync`).
- `.bin` **giống từng byte** bản trước bước này (SHA256 `3921ebd5…`) ⇒ chỉ đổi comment.

#### Bước 3 — nhánh chết và cửa probe

- Rà toàn bộ **196 dòng `#if/#ifdef`** trong `Core/Inc` + `Core/Src`: phần lớn là include guard
  (`#ifndef X_H`) của HAL/CMSIS/CubeMX và **55 `#ifndef` của `tuning.h`** (cửa override `-D`, hợp lệ,
  giữ nguyên).
- **Không có nhánh chết thật** trong code của mình: mọi `#if` còn lại đều là probe đang được test dùng.
  `#ifdef USE_FULL_ASSERT` (main.c) và các khối HAL để nguyên (CubeMX sinh).
- **Gom 3 probe** về một khối ở đầu mỗi module, đặt tên rõ:
  | Module | Probe | Dùng cho |
  |---|---|---|
  | `balance_motion.c` | `TWIP_PROBE_LEAD_CUT` | test A/B luật cũ "cắt tốc độ khi lệch 20 cm" |
  | `balance_turn.c` | `TWIP_PROBE_LATERAL_MEASURED` | test A/B trần ngang theo tốc độ đo |
  | `balance_recovery.c` | `TWIP_PROBE_IMMEDIATE_MANEUVER` | test A/B vào REC1 ngay, không cần 60 ms |
  Tên cũ (`BALANCE_MOTION_LEAD_CUT`, …) vẫn nằm trong `tuning.h` để 9 probe còn `-D` được; probe chỉ là
  alias `#define TWIP_PROBE_x BALANCE_…`.
- `.bin` **giống từng byte** (`3921ebd5…`) ⇒ không đổi mã máy; 38/38 test PASS; 3 bộ mô phỏng trùng.
### Bước 4 — tách hàm trong `robot_fsm.c` (xong 21/11)

- Vòng 200 Hz (178 dòng, một hàm) giờ là **9 hàm `static`** + một `RobotTick` giữ giá trị dùng chung
  trong nhịp; hàm `Robot_ControlLoop_200Hz` chỉ còn 10 dòng gọi đúng thứ tự 16 bước:

  | Hàm | Bước | Việc |
  |---|---|---|
  | `robot_tick_start` | 1 | DWT, khoảng cách nhịp, `loop_count`, lấy lệnh |
  | `robot_read_inputs` | 2–4 | đọc BMI160, lọc bù, encoder, `x_actual` (IMU lỗi → cắt 32) |
  | `robot_check_safe_state` | 5–7 | xoá đích, STOP khẩn, cấu hình, NaN (cắt 512) |
  | `robot_update_arm_gate` | 8 | cổng ARM 200 ms (góc/gyro/bánh/nhịp) |
  | `robot_handle_arm` | 9 | nhận ARM, reset tích phân/encoder/mốc |
  | `robot_run_veto_chain` | 10 | chuỗi veto: heartbeat, va đập, capture, REC1, governor, quỹ đạo, góc/quãng, interlock |
  | `robot_apply_balance_law` | 11–14 | lõi R12 (nguồn PWM duy nhất), bù bánh trái, ghi PWM / dừng |
  | `robot_check_deadline` | 15 | nhịp > 4 ms → cắt 64 |
  | `robot_emit_telemetry` | 16 | khung 122 B mỗi 4 nhịp |

- **Bằng chứng tương đương**: đối chiếu **từng câu lệnh và thứ tự** giữa thân hàm cũ và 9 thân hàm mới
  (bỏ tiền tố `t->`, thay khai báo cục bộ bằng trường `RobotTick`, `break`→`return`): **9 236 ký tự đã
  chuẩn hoá trùng khớp hoàn toàn**. Không mất, không thêm, không đảo câu lệnh nào.
- Build CubeIDE: **0 lỗi, 0 cảnh báo**; `text 84 072 / data 500 / bss 7 572` (bss +56 B do `RobotTick`),
  `.bin` 84 580 B, SHA256 `4cb735bd…` — **khác byte** bản trước, đúng như dự kiến của bước 4 (gọi hàm thay
  vì inline), nên bằng chứng phải là đối chiếu câu lệnh chứ không phải hash.
- **Giới hạn phải nói rõ**: `robot_fsm.c` **không có test tự động** (38 bộ test và mô phỏng chỉ dùng
  header), nên bước này **không có lưới test**. Muốn tin hoàn toàn phải có một lượt chạy thật (hoặc kê
  cao bánh) với log trước khi nạp; đây là lý do vẫn **không bump build ID và không nạp board**.
- **Không làm**: không đổi thứ tự 16 bước, không đổi câu lệnh nào, không bump, không nạp, không push.

## 6. Tổng kết dọn code

| Bước | Nội dung | Trạng thái | Bằng chứng |
|---|---|---|---|
| 1 | `tuning.h` một chỗ chỉnh thông số | ✅ `de3b7c8` | `.bin` giống từng byte |
| 2 | Comment bỏ lịch sử | ✅ `4b3ec41` | `.bin` giống từng byte |
| 3 | Nhánh chết / gom probe | ✅ `4b3ec41` | 3 probe `TWIP_PROBE_*`, không có nhánh chết |
| 4 | Tách hàm `robot_fsm.c` | ✅ bước này | đối chiếu câu lệnh 9 236 ký tự khớp |
| 5 | Dọn thư mục sinh ra | ✅ `da4583d` | 1,8 GB, thêm `Cay_Thu_Muc.md` |
| 6 | Tách module `.h` → `.c` | ✅ `c8c299c`, `d42131d`, `70a89a2` | 3 bộ mô phỏng 73/73 file trùng |

> **Đã đóng gói RT45 (21/11), sau đó xoá khỏi cây** khi chuyển sang nạp bằng IDE — gói còn trong git history (`0e7b8d1`).
> Xem mục 4 của README gói để chạy lượt nghiệm thu đầu tiên (vì `robot_fsm.c` không có test tự động).
