# Report — Week 2 / Ex4

`student_id`: 2413284    Commit của PREDICTION.md: fa5f79f0f93a65e37508874c4ba82dcde51ab518

## 1. Bảng so sánh RAM
| | Bytes thực tế | Cách tính |
|---|---|---|
| Event group (4 điểm đồng bộ) | 24 | Một object: sizeof(EventGroup_t) = 24 |
| Event group (3 điểm) + notification (1 điểm) | 24 | Một Event Group 24 bytes + 0 byte cấp phát thêm cho Notification |
| Chênh lệch | 0 | 24 − 24 = 0 |

## 2. Bảng so sánh tốc độ đánh thức
| Cách | Thời gian đánh thức (cycle) | Thời gian đánh thức (us) |
|---|---|---|
| Event-group-bit (producer_1 hoặc 2) | 166930 | 2318 |
| Task notification (producer_0) | 153099 | 2126 |

Mô tả cách đo (2 điểm DWT: lúc gọi set/notify, lúc sync_task bắt đầu chạy): dùng DWT lấy t0 ngay trước xTaskNotifyGive()/xEventGroupSetBits() và t1 ngay khi sync_task xác nhận đủ mọi điều kiện đồng bộ để chạy

## 3. Khi nào KHÔNG dùng được notification
Tình huống bạn dự đoán ở PREDICTION.md, kiểm chứng lại: đúng hay sai, vì sao.

- Tình huống đã đúng đối với cách dùng `xTaskNotifyGive()` / `ulTaskNotifyTake()` để đếm số lần báo. Vì count không phân biệt nguồn gửi, điều kiện `count >= 4` không bảo đảm đủ bốn nguồn. Do đó, không thể thay trực tiếp Event Group bằng `xTaskNotifyGive()` / `ulTaskNotifyTake()`.

## 4. Timeout
Log/ảnh một lần `sync_task` bị timeout (cố tình làm 1 producer chậm hơn
`SYNC_TIMEOUT_MS`), và giá trị trả về của `xEventGroupWaitBits` lúc đó.

```
SYNC,t_ms=176953,period_us=174003,period_valid=1
TIMEOUT,bits=0x7
SYNC,t_ms=177127,period_us=173996,period_valid=1
TIMEOUT,bits=0x7
SYNC,t_ms=177301,period_us=174003,period_valid=1
TIMEOUT,bits=0x7
SYNC,t_ms=177475,period_us=173996,period_valid=1
TIMEOUT,bits=0x7
```
- Producer 3 được đặt delay 170 ms, vượt timeout 120 ms. Giá trị trả về 0x07 cho thấy thiếu bit 3, do đó chưa đủ mask 0x0F.

## 5. Lệch so với dự đoán
Mỗi mục lệch + nguyên nhân vật lý.

| Nội dung | Dự đoán | Đo được | Giải thích |
|---|---|---|---|
| RAM Event Group | 24 bytes | 24 bytes | Khớp với dự đoán vì EventBits_t 4 byte và List_t 20 byte |
| RAM cấp phát thêm khi dùng Notification | 0 | 0 | Các trường Notification đã có trong TCB. Event Group vẫn tồn tại cho các producer còn lại |
| Thời gian Notification so với Event Group | Notification ngắn hơn | Notification ngắn hơn Event Group | Khớp với dự đoán vì event group phải kiểm tra điều kiện task còn notify đi thẳng vào TCB của task để báo |

## 6. Câu hỏi đánh đổi
Với `EVENT_BIT_COUNT` của **bạn**, nếu tăng thêm 1 bit (1 producer nữa) thì chu
kỳ đồng bộ trung bình đo được thay đổi thế nào? Trả lời bằng số từ dữ liệu của
bạn, rồi đo lại để kiểm chứng.

- Dự đoán: Với 4 producer, chu kỳ đồng bộ trung bình là 6909,488 µs. Trước khi thử 5 producer, em dự đoán chu kỳ có thể tăng do sync_task phải chờ thêm một nguồn nữa.

- Kiểm chứng:

| Cấu hình | Số mẫu | Chu kỳ trung bình |
|---|---|---|
| 4 producer | 555 | 6909,488 us |
| 5 producer | 555 | 6979,771 µs | 
| Chênh lệch | | +70,283 µs ≈ +1,017% |

- Kết luận: Chu kỳ trung bình tăng khoảng 70,283 µs, tương đương khoảng 1%, phù hợp với dự đoán tăng nhẹ.
