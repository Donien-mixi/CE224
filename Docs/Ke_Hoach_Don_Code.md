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

Bước 6 (tuỳ chọn) đang làm **từng module, mỗi module một commit nhỏ**. **Không bump build ID, không
nạp board**: đây là refactor, hành vi phải y nguyên. Bước 5 (dọn thư mục) đã xong ở commit `da4583d`.

| Module | Trạng thái | Commit | Bằng chứng |
|---|---|---|---|
| 1/3 `balance_recovery` | xong 21/11 | `c8c299c` | 38/38 bộ test chạy được; mô phỏng @0,80 12/12, số từng ca trùng |
| 2/3 `balance_motion` | xong 21/11 | commit sau `c8c299c` | 38/38 bộ test + 9 probe; hex cùng kích thước; 3 bộ mô phỏng trùng từng byte |
| 3/3 `balance_turn` | **chưa làm** | — | 19 bộ test include `balance_turn.h` |

### 2/3 — `balance_motion` (bằng chứng chi tiết)

- **Tách**: `Core/Src/balance_motion.c` chứa 8 hàm (`BeginBrake`, `BeginBrakeHold`, `Cancel`, `Fault`,
  `Impact`, `corner_budget`, `maxhead`, `maxhead_next`); `balance_motion.h` (340 → 275 dòng) chỉ còn
  thông số (giữ nguyên tên + `#ifndef`), struct, `Clear`, prototype và `BalanceMotion_Update`.
- **`BalanceMotion_Update` vẫn inline**: caller chèn hook governor qua macro `BALANCE_MOTION_FILTER` /
  `BALANCE_MOTION_ANCHOR` **trước khi** include (`robot_fsm.c:30`, `Simulation/current/motion_native.c:4`,
  10 bộ test). Đưa nốt vào `.c` thì phải đổi hook thành con trỏ hàm — để dành khi tách `balance_turn`.
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
