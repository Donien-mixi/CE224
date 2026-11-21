# RT45 — thay đổi tổng thể của đợt dọn code (để review)

Tài liệu này tóm tắt **toàn bộ** thay đổi của đợt "clean code" từ commit `0946d2d` (trước khi dọn) đến
`d3d0068` (log RT45 đầu tiên). Mục tiêu: để người đọc review được nhanh, biết **cái gì đổi**, **cái gì
không đổi**, và **bằng chứng** cho từng bước.

## 1. Phạm vi

| Mục | Giá trị |
|---|---|
| Số commit | **10** (`da4583d` + 8 commit code + `0e7b8d1` đóng gói + `d3d0068` log) |
| Tệp thay đổi | **68** tệp (code, test, docs, gói firmware) |
| Dòng | **+7 204 / −1 259** (chưa kể binary đóng gói) |
| Build ID | chỉ đổi **một lần**, ở bước đóng gói: RT44 → **RT45-CLEAN / NET45** |
| Hành vi điều khiển | **không đổi** (xem mục 4–5) |

## 2. Cấu trúc trước / sau

| Tệp | Trước | Sau | Ghi chú |
|---|---:|---:|---|
| `balance_motion.h` | 340 | **84** | chỉ còn khai báo + thông số trỏ `tuning.h` |
| `balance_motion.c` | — | **263** | thân hàm (mới) |
| `balance_turn.h` | 325 | **47** | |
| `balance_turn.c` | — | **175** | (mới) |
| `balance_recovery.h` | 186 | **46** | |
| `balance_recovery.c` | — | **128** | (trước đây đã có một phần) |
| `balance_governor.h` | 188 | **129** | **vẫn inline** — xem mục 6 |
| `balance_profile.h` | 117 | **67** | chỉ còn build ID, R12 đóng băng, ARM, capture, chốt an toàn |
| `tuning.h` | — | **184** | 55 thông số, 6 nhóm (mới) |
| `robot_fsm.c` | 420 | **503** | vòng chính 178 dòng → **10 dòng** + 9 hàm có tên |

## 3. Năm thay đổi lớn

### 3.1 Tách `.h` → `.c` cho 3 module quỹ đạo/giám sát

Trước: hàm `static inline` nằm trong header, ai include cũng biên dịch lại. Sau: khai báo trong `.h`,
thân trong `.c`; mỗi hàm một comment "để làm gì / gọi khi nào".

```c
/* trước — balance_motion.h (trích) */
#define BALANCE_MOTION_SPEED (s->speed_preset==2? .90f:s->speed_preset? .80f:.50f)
/* RT19: third preset is0.80m/s... (7 dòng kể log) */
static inline int BalanceMotion_Update(...){ ... 178 dòng ... }

/* sau — tuning.h (trích) */
#define BALANCE_MOTION_SPEED (s->speed_preset==2? .90f:s->speed_preset? .80f:.50f)  // tốc độ đặt 50/80/90 cm/s theo mốc (dùng biến s của module gọi)

/* sau — balance_motion.h (trích) */
int BalanceMotion_Update(BalanceMotion *s, bool allow, uint32_t word, float position,
                         float pitch, float gyro, float speed, uint32_t now);
```

### 3.2 Một chỗ chỉnh thông số: `tuning.h`

**55 macro** từ 5 tệp, chia 6 nhóm, mỗi dòng `#define TÊN giá_trị  // đơn vị — để làm gì`:
tốc độ/ramp dọc, cua, REC1, governor, giám sát tốc độ, giới hạn chạy + bù bánh trái.
Giữ nguyên tên + `#ifndef` để 9 probe còn override bằng `-D`. `balance_profile.h` chỉ còn thứ **không
được chỉnh bừa**: build ID, luật R12, hiệu chuẩn cảm biến, cổng ARM, capture, chốt góc/quãng, heartbeat.

### 3.3 Hook governor: macro của caller → 3 hàm yếu

Trước: mỗi nơi dùng phải `#define BALANCE_MOTION_FILTER(...) BalanceGovernor_Step(&gov,...)` **trước khi**
include header — macro không đi qua được translation unit. Sau: `balance_motion.c` có bản yếu
(`FilterHook`, `AnchorHook`, `GovernorActive`), `robot_fsm.c` / 10 bộ test / harness mô phỏng cài bản mạnh.

```c
/* robot_fsm.c — sau */
float BalanceMotion_FilterHook(float old,float target,float rate,int wanted,float pitch,float gyro){
    return BalanceGovernor_Step(&bal_governor,old,target,rate,wanted,pitch,gyro);
}
```

Lỗi đã bắt được nhờ test: bản đầu thiếu `GovernorActive` nên khối `#ifndef BALANCE_MOTION_FILTER` (reset
`v_ref` khi bắt đầu lệnh mới) chạy sai nhánh — `test_rt18_launch` phát hiện (`v_ref` = 0 thay vì 1,25e-5).

### 3.4 Comment: bỏ lịch sử, giữ lý do

- **31 nhãn "RTxx"** trong comment code được viết lại thành 1–3 dòng nêu *để làm gì / đánh đổi gì*
  (`robot_fsm.c` 9 khối, `balance_governor.h` 7, `balance_interlock.h`, `balance_trim.h`, `bmi160.c`,
  `usart.c`, `tuning.h`).
- Còn đúng 1 chỗ chứa "RT44": literal build ID (phải giữ).
- Comment tiếng Việt cho phần mới; các khối cũ trong `balance_governor.h` vẫn tiếng Anh (xem mục 6).

### 3.5 Vòng 200 Hz: 178 dòng → 9 hàm + 10 dòng điều phối

```c
void Robot_ControlLoop_200Hz(void) {
    robot_tick_start();        /* 1: DWT, spacing, loop_count, lệnh */
    robot_read_inputs();       /* 2–4: IMU, lọc bù, encoder, x_actual */
    robot_check_safe_state();  /* 5–7: STOP khẩn, cấu hình, NaN */
    robot_update_arm_gate();   /* 8: cổng ARM 200 ms */
    robot_handle_arm();        /* 9: nhận ARM, reset tích phân/mốc */
    if(s_robot.state==ROBOT_STATE_BALANCING) robot_run_veto_chain(); /* 10 */
    robot_apply_balance_law(); /* 11–14: lõi R12 + bù bánh trái + PWM */
    robot_check_deadline();    /* 15: nhịp > 4 ms thì cắt 64 */
    robot_emit_telemetry();    /* 16: khung 122 B mỗi 4 nhịp */
}
```

Giá trị dùng chung giữa các bước nằm trong một `RobotTick` (begin, spacing, cmd, imu, valid, norm, finite,
just_armed, execution) thay vì 8 biến rời.

## 4. Những gì **không** đổi

- Luật lực R12: `CAT_K`, `CAT_PARAM`, observer 14 trạng thái, `cat_allocate`, `CAT_VNL/FRICTION/START`.
- CF alpha 0,998; zero lắp 2,16694264915°; `BALANCE_PPR` 898,225; dấu encoder; `METERS_PER_TICK`.
- Khung telemetry **122 B**, 40 cột, 50 Hz; `$BINFO`/`$MOVE8`, lease 500 ms, epoch/sequence.
- Cổng ARM ±60°, ready 200 ms, heartbeat 10 s, các chốt REC1/interlock/governor/trim, trần cua, trần MAX.
- **Toàn bộ giá trị 55 thông số**: không đổi một số nào.
- Thứ tự 16 bước của vòng 200 Hz.

## 5. Chuỗi bằng chứng

| Bước | Bằng chứng | Kết quả |
|---|---|---|
| 5 (dọn thư mục) | — | 1,8 GB sinh ra đã xoá, thêm `Docs/Cay_Thu_Muc.md` |
| 6 (tách .c) | 38/38 test; 3 bộ mô phỏng | **73/73 tệp trùng** từng byte với trước khi tách |
| 1 (`tuning.h`) | `.bin` build lại | **giống từng byte** `3921ebd5…` (chỉ di chuyển macro) |
| 2 (comment) | `.bin` build lại | **giống từng byte** |
| 3 (probe) | `.bin` build lại | **giống từng byte** |
| 4 (`robot_fsm.c`) | đối chiếu câu lệnh | **9 236 ký tự chuẩn hoá trùng khớp**, không mất/đảo câu lệnh |
| Đóng gói RT45 | cleanBuild trong workspace sạch | **hex + bin tái lập đúng** hash đã đóng gói; 7/7 binary + 20/20 nguồn khớp |
| Trên xe 21/11 | 30 000 mẫu / 600 s | `fault=0`, `exec` p95 **660 µs** (+12 µs), REC1 5/5 tự thoát, MAX 90 đạt 0,883 m/s |

## 6. Còn chỗ nào "chưa giống người viết" (để anh xem xét)

1. **`balance_governor.h` vẫn inline** (129 dòng). Ba module khác đã tách `.c`; governor là chỗ còn lại.
   Tách được nhưng phải thêm một tệp vào makefile/mô phỏng — việc của một phiên riêng.
2. **`balance_interlock.h` và `balance_trim.h` vẫn inline** — nhưng mỗi tệp 26–40 dòng, inline là hợp lý.
3. **Comment trộn ngôn ngữ**: phần mới tiếng Việt, phần cũ trong governor/interlock tiếng Anh.
4. **Ba macro dùng biến của module gọi** (`BALANCE_MOTION_SPEED`, `BALANCE_MOTION_ACCEL`,
   `BALANCE_TURN_PURE/DIAGONAL_RATE`) vẫn là macro vì đổi sang hàm sẽ mất khả năng override `-D` của probe.
5. **Script test viết nén** (mỗi tệp `test_*.cjs` gần như một dòng) — đây là phần "ít giống người" nhất
   còn lại, nhưng sửa 38 tệp không đổi giá trị kỹ thuật.
6. **Docs theo vòng** (RT22 → RT45) dài và có lịch sử — cố ý giữ làm bằng chứng, không phải code.

## 7. Cách review nhanh (≈30 phút)

1. `Xe_Can_Bang/Core/Inc/tuning.h` — xem 55 thông số có đúng đơn vị/ý nghĩa không (một chỗ duy nhất).
2. `Xe_Can_Bang/Core/Src/robot_fsm.c` dòng 491 — vòng chính 10 dòng, đọc như mục lục.
3. `Xe_Can_Bang/Core/Src/balance_turn.c` — luật cua (phần khó nhất) đọc liền mạch.
4. `git show c8c299c`, `d42131d`, `70a89a2`, `de3b7c8`, `4b3ec41`, `9895be9` — từng bước một, mỗi commit
   có bằng chứng riêng trong message.
5. `Docs/Ke_Hoach_Don_Code.md` mục 5 — bảng tiến độ + số đo của từng bước.
