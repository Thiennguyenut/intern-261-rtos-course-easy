# Report — Week 1 / Ex2

`student_id`: 2413284    Commit của PREDICTION.md: 9cdc77b0ac84f82b5183f10d0a77ef1be353bdd4

## 1. CPU share đo được
| Cấu hình | A (%) | B (%) | C (%) | Time slice đo được | Ảnh |
|---|---|---|---|---|---|
| B2: preempt=1, ưu tiên khác nhau | 0% | 0% | 100% | — | ex2_b2.png |
| B3: preempt=0, ưu tiên khác nhau | 0% | 0% | 100% | — | ex2_b3.png |
| B4: preempt=1, cùng ưu tiên | ~33% | ~33% | ~33% | 0,998 ms | ex2_b4.png |
| B5: sau khi sửa starvation | ~38,2% | ~18,2% | ~42,2% | — | ex2_b5.png |

Cách tính CPU share từ ảnh PulseView (mô tả ngắn): CPU share (%) = (Tổng thời gian task chiếm CPU trong 1 chu kỳ / Tổng thời gian của 1 chu kỳ) x 100%.

## 2. Preemption nhìn thấy ở đâu
Trên ảnh **bước 4** (cùng ưu tiên, round-robin), chỉ ra ít nhất **2 thời điểm**
preemption xảy ra. Với mỗi thời điểm: dấu thời gian, task bị cắt, task chiếm CPU,
và vì sao (hết time slice, không phải block).

| t (us) | Task bị preempt | Task chiếm CPU | Lý do |
|---|---|---|---|
| 1362,33 | A | B | Tick chuyển lượt giữa các task cùng priority |
| 2295 | B | C | Tick chuyển lượt giữa các task cùng priority |
| 3226,33 | C | A | Tick chuyển lượt giữa các task cùng priority |

Ở **bước 2** (ưu tiên khác nhau, đều busy-loop không block): giải thích vì sao ảnh
của bạn cho thấy **0 thời điểm preemption**, dù chạy bao lâu đi nữa — dựa vào điều
kiện preemption của FreeRTOS, không phải "vì ảnh chụp chưa đủ dài".
**Trả lời:**
- vì ở bước 2 task C có priority 4 và luôn Ready vì chạy vòng lặp không block/yield. Task A và B cũng Ready nhưng có priority thấp hơn task C. Do đó mỗi lần scheduler quyết định, task C vẫn là task Ready ưu tiên cao nhất. Vì vậy không có preemption giữa các task.

## 3. Time slice
Time slice đo được ở bước 4: 0,998 ms. `configTICK_RATE_HZ` = 1000.
Sai số: 0,2%. Giải thích sai số: Vì configTICK_RATE_HZ = 1000, nên time slice danh định là (1/1000) giây = 1 ms. Time slice đo được lệch so với danh định là 0,002 ms nên sai số là (0,002/1) * 100% = 0,2%. Nguyên nhân có thể là do Logic Analyzer có tốc độ lấy mẫu chưa đủ, chưa chính xác.

## 4. DWT wrap-around
Tại sao so sánh `DWT->CYCCNT > target` bị sai sau ~59 giây, và cách viết đúng:
**Trả lời:**
- DWT->CYCCNT là bộ đếm 32 bit. Ở 72 MHz, nó quay vòng sau khoảng (2^32/72,000,000 = ~59,65) giây. Nếu so sánh CYCCNT > target, phép cộng hoặc counter có thể tràn về 0 và làm điều kiện sai gần điểm quay vòng.
- Cách viết đúng là (uint32_t)(DWT->CYCCNT - start). Phép trừ vẫn cho đúng số chu kỳ đã trôi qua qua một lần tràn, miễn khoảng chờ nhỏ hơn một vòng counter. DWT đo thời gian trôi qua; nếu task bị preempt, counter vẫn tiếp tục tăng.

## 5. Sửa starvation
Cách tôi chọn: thêm vTaskDelay(1) ở task B và C.
Vì sao nó hoạt động (dựa vào trạng thái task trong scheduler).
**Trả lời:**
- Vì Khi C Blocked, B được chọn. Lúc này task B chạy xong sẽ bị đưa vào trạng thái Blocked; khi đó B và C cùng Blocked, A là task Ready ưu tiên cao nhất nên được chạy.
- Đánh đổi: Task B, C không còn chạy nhiều vòng liên tục nên không còn chiếm CPU liên tục nên mức sử dụng CPU giảm; thời gian CPU được phân bổ cho A tăng lên. Tuy nhiên, vTaskDelay() làm tăng độ trễ phản hồi và độ trễ lập lịch phụ thuộc vào chu kỳ tick.

## 6. Lệch so với dự đoán
Mỗi mục lệch + nguyên nhân vật lý.
**Trả lời:**
- Bước 2: Khớp với prediction. Vì task C luôn ở trạng thái running nên CPU đo được là 100%
- Bước 3: Khớp với prediction. Vì task C được chọn lúc scheduler bắt đầu rồi không block/yield nên CPU vẫn đo được là 100%
- Bước 4: Time slice bị lệch nhẹ so với prediction. Nguyên nhân có thể là độ phân giải lấy mẫu của Logic Analyzer,độ trễ xử lý của SysTick/scheduler.

## 7. Câu hỏi đánh đổi
Với **bộ ưu tiên của bạn**, nếu `TASK_C_WORK_US` tăng gấp 3 lần thì bảng CPU share ở
bước 2 thay đổi thế nào? Trả lời bằng số suy ra từ dữ liệu của bạn, rồi **đo lại để
kiểm chứng**. Nếu lệch, giải thích.
**Trả lời:**
- Dự đoán: Khi `TASK_C_WORK_US` tăng gấp 3 lần thì CPU share giữa ba task vẫn A= 0%, B= 0%, C= 100%. Vì C luôn Ready và ưu tiên cao nhất nên làm việc lâu hơn không tạo cơ hội cho task A và B.
- Kiểm chứng:
ex2_b6.png

- Đúng với dự đoán, khi tăng thời gian làm việc của C từ 420 lên 1260 µs, CPU share giữa ba task vẫn là A= 0%, B= 0%, C= 100%. Kết quả đo được cho thấy thời gian ~1261.6667us, sai số nhỏ ~1.6667us. Nguyên nhân có thể là độ phân giải lấy mẫu của Logic Analyzer.
