# Dự đoán — Week 2 / Ex2

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2413284   `WRITER_COUNT` = 3   `MSG_LEN` = 40 bytes

1. Ở `UART_GUARD_MODE = 0`: dự đoán lỗi lẫn lộn sẽ xuất hiện sau khoảng bao
   nhiêu dòng log? Vì sao MSG_LEN và WRITER_PERIOD_MS của bạn ảnh hưởng đến
   con số này?
   - Với giả thiết 3 writer cùng priority 1; preemption và time slicing bật; tick 1 ms. Mode 0–2 writer gọi HAL từng byte còn mode 3 writer gửi queue, gatekeeper gọi HAL từng byte.
   - Với UART 115200 baudrate 8N1, mỗi byte cần 10 bit nên thời gian truyền lý tưởng là: $T_{\text{UART}} = \frac{40 \times 10}{115200} \approx 3.472\text{ms}$
   - Giả sử 3 writer đều được tạo thành công và ở trạng thái Ready. Em dự đoán lỗi có thể xuất hiện ngay trong đợt gửi đầu, khoảng 1–3 dòng log đầu tiên.
   - Vì một message cần tối thiểu 3,472 ms, dài hơn 1 tick = 1 ms. Khi writer đầu chưa gửi hết message, scheduler có thể chuyển sang writer khác cùng priority đang Ready. Vì UART được gọi theo từng byte và không khóa toàn message, writer khác có cơ hội gửi byte của mình giữa các phần của message đang dở. Tuy nhiên, nếu chuyển task xảy ra trong một lần gọi HAL đang bận, writer tiếp theo có thể nhận HAL_BUSY.
   - MSG_LEN : ảnh hưởng đến thời gian hoàn thành message, tức là đi qua nhiều hoặc ít tick hơn, điều đó đồng nghĩa với việc cơ hội bị writer khác chen vào cũng bị ảnh hưởng theo.
   - WRITER_PERIOD_MS : ảnh hưởng đến thời gian writer quay lại gửi sau khi hoàn thành message, cũng ảnh hưởng đến việc khả năng tranh chấp.
2. Dự đoán thứ tự **độ trễ in** (latency) từ thấp đến cao giữa mutex / critical
   section / gatekeeper. Vì sao?
   - Do latency được tính từ ngay trước khi writer yêu cầu gửi đến khi message đó truyền xong. Giả sử đợt gửi đầu, ba writer ban đầu đều Ready và queue gatekeeper ban đầu rỗng, em dự đoán:
      $L_{\text{critical}} \approx L_{\text{gatekeeper}} < L_{\text{mutex}}$
   - Critical section: writer truyền trọn message trước khi task khác được chuyển sang chạy. Latency mỗi message gần thời gian truyền 40 byte cộng overhead.
   - Gatekeeper: priority 2 cao hơn writer priority 1, nên có thể chạy ngay sau enqueue. Latency gần thời gian truyền cộng overhead queue/context switch.
   - Mutex: writer khác có thể chạy ở tick tiếp theo, phát yêu cầu rồi Blocked chờ mutex; latency có thêm thời gian chờ.
3. Với critical section, task nào trong hệ thống có nguy cơ bị ảnh hưởng xấu nhất nếu đoạn in quá dài? (gợi ý: nghĩ đến ngắt, không chỉ task)
   - Em dự đoán cơ chế thời gian và scheduling dựa trên SysTick bị ảnh hưởng trực tiếp, kéo theo các writer còn lại bị chạy muộn.
   - taskENTER_CRITICAL() nâng BASEPRI để mask các ngắt có số priority từ 5 đến 15. SysTick và PendSV được FreeRTOS đặt ở priority thấp nhất nên bị trì hoãn trong critical section. Với message 40 byte, UART 115200 baud, 8N1, thời gian giữ critical dự kiến gần 3,472 ms. Dài hơn 1 tick = 1 ms. Bộ đếm phần cứng SysTick vẫn hoạt động, nhưng handler chưa được phục vụ. Các lần hết chu kỳ trong lúc bị mask không được lưu thành một hàng đợi để tự bù đầy đủ sau đó, nên tick FreeRTOS có thể chậm so với thời gian thực.
   - Ngoài ra, TIM4 cung cấp HAL tick đang được cấu hình priority 15, nên cũng bị mask với giả thuyết trên. HAL_IncTick() bị trì hoãn, ảnh hưởng timeout dựa trên HAL_GetTick().

4. RAM dùng thêm của gatekeeper so với mutex: dự đoán tăng hay giảm, vì sao?
   - Em dự đoán tăng, vì Mutex dùng cấu trúc queue với item size bằng 0 khoảng 72 byte, không cần vùng chứa message còn gatekeeper cần một queue và một task riêng cụ thể như sau:
   - TCB gatekeeper: 84 bytes
   - Stack gatekeeper: 800 bytes
   - Queue control: 72 bytes
   - Queue chứa message: WRITER_COUNT × MSG_LEN = 3 x 40 = 120 bytes
   - Tổng gatekeeper chưa tính cấp phát: 1076 byte còn mutex 72 byte → tăng 1004 byte

5. Nếu `WRITER_COUNT` tăng gấp đôi, cách nào trong 3 cách bị ảnh hưởng về độ trễ
   nhiều nhất? Vì sao?
   - Em dự đoán Mutex bị ảnh hưởng nhiều nhất khi `WRITER_COUNT` tăng từ 3 lên 6. 
   - Vì nhiều writer có thể phát yêu cầu rồi Blocked chờ cùng mutex. UART chỉ được một writer giữ mutex sử dụng tại một thời điểm. Các writer khác gọi Take khi mutex chưa được giải phóng sẽ chuyển sang Blocked để chờ. Khi nhiều writer cùng yêu cầu gửi, writer được phục vụ về sau phải chờ thêm các message phía trước truyền xong nên latency tăng.
   - Ngoài ra sau khi writer giữ mutex gọi Give, kernel có thể đưa một writer đang chờ sang Ready. Writer đó được chạy khi scheduler lựa chọn. Việc quản lý danh sách chờ, đánh thức task và chuyển ngữ cảnh phát sinh thêm overhead.
   - Vì vậy, latency tăng chủ yếu do chờ UART phục vụ các writer trước, cộng thêm overhead đồng bộ và scheduling. 
