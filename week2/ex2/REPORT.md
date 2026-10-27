# Report — Week 2 / Ex2

`student_id`: 2413284  Commit của PREDICTION.md: a151ddd3317ec8130838984d09cf55fbc1a03d2f

## 1. Lỗi tái hiện (UART_GUARD_MODE = 0)
Log UART dán nguyên văn, đánh dấu vị trí ký tự lẫn lộn:

```
W0:000000:AAW1:000000:BCCCCCCCCCCCCAAAAAAAAAAABBBBBBBBBBBBCCCCCCCCCCCAAAAAAAAAAAABBBBBBBBBBBCCC
AAA
BBBB
W2:000001:CCW0:000001:AAW1:000001:BCCCCCCCCCCCCAAAAAAAAAAABBBBBBBBBBBBCCCCCCCCCCCAAAAAAAAAAAABBBBBBBBBBBCCC
AAA
BBBB
W2:000002:CCW0:000002:AAW1:000002:BCCCCCCCCCCCCAAAAAAAAAAABBBBBBBBBBBBCCCCCCCCCCCAAAAAAAAAAAABBBBBBBBBBBCCC
AAA
BBBB
W2:000003:CCW0:000003:AAW1:000003:BCCCCCCCCCCCCAAAAAAAAAAABBBBBBBBBBBBCCCCCCCCCCCAAAAAAAAAAAABBBBBBBBBBBCCC
AAA
BBBB
W2:000004:CCW0:000004:AAW1:000004:BCCCCCCCCCCCCAAAAAAAAAAABBBBBBBBBBBBCCCCCCCCCCCAAAAAAAAAAAABBBBBBBBBBBCCC
AAA
BBBB
W2:000005:CCW0:000005:AAW1:000005:BCCCCCCCCCCCCAAAAAAAAAAABBBBBBBBBBBBCCCCCCCCCCCAAAAAAAAAAAABBBBBBBBBBBCCC
AAA
BBBB
W2:000006:CCW0:000006:AAW1:000006:BCCCCCCCCCCCCAAAAAAAAAAABBBBBBBBBBBBCCCCCCCCCCCAAAAAAAAAAAABBBBBBBBBBBCCC
AAA
BBBB
```

Tái hiện lại bao nhiêu lần / bao nhiêu lần thấy lỗi: 3 / 3.
- Ở dòng bắt đầu bằng W2:000001:, header W2 xuất hiện tại ký tự đầu tiên, W0 tại ký tự 13 và W1 tại ký tự 25. W0 và W1 bắt đầu gửi khi message W2 chưa kết thúc, payload C, A, B bị xen kẽ trước CR/LF.

## 2. Bảng so sánh 3 cách
| Cách | Độ trễ in trung bình (us) | Độ trễ in max (us) | Ảnh hưởng task khác | RAM thêm (bytes) |
|---|---|---|---|---|
| Mutex | 5966,92 | 9486,60 | Writer chờ Take trung bình 2486,25 µs, lớn nhất 6009,53 µs | 80 byte heap + 4 byte handle tĩnh |
| Critical section | 3479,16 | 3482,90 | W0 = 7896,32 µs, W1 = 11429,92 µs, W2 = 4366,86 µs nên trung bình khoảng 7897,70 µs | 0 |
| Gatekeeper | 3505,2 | 3510,57 | Gatekeeper priority 2 có thể trì hoãn writer priority 1 khi polling UART. Trung bình các khoảng delay thực tế lớn nhất của 3 writer: 8325,82 µs | 1104 byte heap + 4 byte handle tĩnh |

Cách đo độ trễ in (mô tả ngắn, kèm ảnh logic analyzer nếu dùng pin đánh dấu):
- Sử dụng bộ đếm chu kỳ CPU DWT->CYCCNT của Cortex-M3, với tần số CPU 72 MHz. Độ trễ in được tính từ lúc writer yêu cầu gửi đến khi UART truyền xong byte cuối, xác nhận bằng cờ TC:
    - Latency (µs) = (uint32_t)(t_end − t_start) / 72,0.
    - Mutex: lấy t_start ngay trước xSemaphoreTake(), lấy t_end sau khi truyền xong và trước xSemaphoreGive().
    - Critical section: lấy t_start ngay trước taskENTER_CRITICAL(), lấy t_end sau khi truyền xong và trước taskEXIT_CRITICAL().
    - Gatekeeper: writer lưu t_start trước xQueueSend(). Gatekeeper nhận message, truyền UART và lấy t_end sau khi truyền xong.

## 3. Vì sao critical section nguy hiểm nhất
Giải thích bằng số đo của bạn: đoạn in dài bao nhiêu us khi ở trong critical
section, và điều đó chặn ngắt nào trong hệ thống bao lâu.
- trong critical section đoạn in mất trung bình 3477,91 µs 
- Vì taskENTER_CRITICAL() dùng BASEPRI để mask các ngắt có mức ưu tiên logic từ 5 đến 15 gồm SysTick, PendSV và TIM4. Do đó ngắt SysTick, PendSV và TIM4 bị chặn trong hệ thống đến khi gọi taskEXIT_CRITICAL().
- Critical section khoảng 3,48 ms vượt tick 1 ms nên có thể làm tick bị gộp pending, trì hoãn đánh thức task, timeout và chuyển ngữ cảnh.

## 4. Lệch so với dự đoán
Mỗi mục lệch + nguyên nhân vật lý.
- Theo dự đoán trong PREDICTION.md, thứ tự latency từ thấp đến cao là: Critical section ≈ Gatekeeper < Mutex.
- Kết quả latency trung bình đo được:

| Cách | Latency trung bình |
|---|---|
| Mutex | 5966,92 |
| Gatekeeper | 3505,2 |
| Critical section | 3479,16 |

- Critical section: latency gần thời gian truyền lý tưởng 40 byte ở 115200 baud, 8N1 là 3472,22 µs. Số đo cao hơn khoảng 6,94 µs, do chi phí vào critical, đọc DWT, thực thi hàm truyền và polling UART.
- Gatekeeper: latency cao hơn critical khoảng 26,04 µs, vì phải copy message vào queue, đánh thức/chuyển sang gatekeeper và nhận message trước khi truyền. Gatekeeper priority 2 cao hơn writer priority 1 nên có thể xử lý ngay sau khi được đánh thức, hạn chế thời gian message chờ trong queue.
- Mutex: latency trung bình cao nhất vì ngoài thời gian truyền, writer còn phải chờ lấy mutex. Thời gian chờ đo được trung bình 2486,25 µs, cộng thời gian truyền trung bình 3480,67 µs nên tổng latency khoảng 5966,92 µs.

## 5. Câu hỏi đánh đổi
Với `WRITER_COUNT` của **bạn**, nếu tăng gấp đôi thì cách nào bị ảnh hưởng độ trễ
nhiều nhất? Trả lời bằng số từ dữ liệu của bạn, rồi đo lại để kiểm chứng.
- Em dự đoán mutex bị ảnh hưởng latency nhiều nhất khi tăng số writer từ 3 lên 6. Vì UART chỉ phục vụ một message tại một thời điểm, nhiều writer cùng yêu cầu sẽ phải chờ các message phía trước truyền xong, cộng thêm chi phí đánh thức task và chuyển ngữ cảnh.

- Kiểm chứng: 

| Cách | TB — 3 writer (µs) | TB — 6 writer (µs) | Max — 3 writer (µs) | Max — 6 writer (µs) | 
|---|---|---|---|---|
| Mutex | 5966,92 | 16398,57 | 9486,60 | 20026,03 |
| Critical section | 3479,16 | 3477,2 | 3482,9 | 3483,13 |
| Gatekeeper | 3505,2 | 3505,56 | 3510,57 | 3512,82 |

- Kết luận: Ở cách Mutex latency trung bình tăng khoảng 10431,65 µs, tương đương 2,75 lần, latency max tăng khoảng 2,11 lần. Khớp với dự đoán về thời gian chờ mutex tăng khi có nhiều writer tranh UART.