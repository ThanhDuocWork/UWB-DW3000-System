5. Trilateration
6. 6. Antenna Delay Calibration
7. NLOS / LOS
LOS  = Line of Sight
NLOS = Non-Line of Sight

LOS:

Anchor nhìn thẳng Tag, ít vật cản

NLOS:

Có tường/người/kim loại/vật cản giữa Anchor và Tag
8. Channel / Preamble / Data Rate / PRF

DW3000 cần cấu hình radio.

Các khái niệm cần biết:

UWB channel
preamble length
SFD
data rate
PAC size
STS
TX power

9. IRQ / RX / TX event

Khi làm firmware, cần hiểu event của DW3000:

TX done
RX good frame
RX timeout
RX error
RX preamble detect
SFD timeout

Các event này thường đi qua chân:

IRQ

Firmware sẽ đọc status register của DW3000 để biết chuyện gì xảy ra.

10. MAC / Frame Format

Bạn cần tự định nghĩa hoặc dùng frame format có sẵn:

source address
destination address
sequence number
message type
timestamp field
checksum

Ví dụ message type:

POLL
RESPONSE
FINAL
REPORT
11. Scheduling / Collision Avoidance

Khi chỉ có:

1 tag + 1 anchor

thì đơn giản.

Nhưng khi có:

nhiều tag + nhiều anchor

sẽ có vấn đề:

nhiều thiết bị gửi cùng lúc
packet collision
timeout
trùng slot

Cần học:

TDMA
time slot
anchor schedule
tag polling schedule
random backoff
12. Filtering

Khoảng cách UWB sẽ có nhiễu.

Cần học các filter cơ bản:

moving average
median filter
low-pass filter
Kalman filter
outlier rejection

Ví dụ:

Raw distance:
1.20, 1.22, 1.19, 3.80, 1.21

3.80 là outlier, cần loại bỏ.
9. Roadmap học theo thứ tự
Giai đoạn 1: hiểu để chạy 2 board
1. UWB basic
2. Anchor / Tag
3. TWR
4. DW3000 SPI + IRQ
5. Poll / Response / Final
6. Read timestamp
7. Calculate distance
Giai đoạn 2: đo ổn định
1. DS-TWR
2. timeout / retry
3. antenna delay calibration
4. filter distance
5. log/debug frame
Giai đoạn 3: định vị
1. nhiều anchor
2. anchor map
3. trilateration
4. position filter
5. NLOS handling
Giai đoạn 4: product thật
1. scheduling
2. multi-tag
3. config storage
4. telemetry
5. gateway Pi4
6. diagnostics
7. OTA/update
10. Tóm tắt cực ngắn
Ranging = đo khoảng cách

Anchor = node cố định, biết vị trí

Tag = node di động, cần đo khoảng cách hoặc tính vị trí

DW3000 = chip UWB giúp gửi/nhận packet và lấy timestamp cực chính xác

TWR = protocol đo khoảng cách bằng cách gửi qua lại packet

Trilateration = dùng khoảng cách tới nhiều anchor để tính vị trí

Calibration + filtering = bắt buộc nếu muốn đo ổn định