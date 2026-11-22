#include <Arduino.h>
#include <DHT20.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>

namespace
{

constexpr uint8_t ledPin = 48;
constexpr uint8_t sensorDataPin = 11;
constexpr uint8_t sensorClockPin = 12;
constexpr unsigned long sensorReadIntervalMilliseconds = 2000;
constexpr unsigned long webServerResponseDelayMilliseconds = 2;
constexpr unsigned long accessPointStatusIntervalMilliseconds = 10000;
constexpr char accessPointName[] = "YoloUNO-IoT-Lab";
constexpr char accessPointPassword[] = "iot12345";

DHT20 temperatureHumiditySensor;
WebServer webServer(80);

bool ledIsOn = false;
bool sensorDataIsValid = false;
bool webServerIsReady = false;
float temperatureCelsius = 0.0F;
float relativeHumidity = 0.0F;
unsigned long lastSensorReadMilliseconds = 0;
unsigned long lastAccessPointStatusPrintMilliseconds = 0;

const char webPage[] PROGMEM = R"HTML(
<!doctype html>
<html lang="vi">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="theme-color" content="#0b1220">
    <title>YoloUNO | Bảng điều khiển</title>
    <style>
        :root
        {
            color-scheme: dark;
            font-family: Inter, ui-sans-serif, system-ui, -apple-system,
                BlinkMacSystemFont, "Segoe UI", sans-serif;
            color: #eef4ff;
            background: #0b1220;
            font-synthesis: none;
            text-rendering: optimizeLegibility;
            -webkit-font-smoothing: antialiased;
            --muted: #91a0b8;
            --panel: rgba(18, 29, 48, 0.88);
            --panel-border: rgba(154, 176, 210, 0.13);
            --accent: #70e1c1;
            --blue: #79a8ff;
        }

        *
        {
            box-sizing: border-box;
        }

        body
        {
            min-width: 320px;
            min-height: 100vh;
            margin: 0;
            padding: 40px 20px;
            background:
                radial-gradient(
                    ellipse at 12% 0%,
                    rgba(49, 104, 151, 0.24),
                    transparent 38rem),
                radial-gradient(
                    ellipse at 100% 80%,
                    rgba(33, 135, 117, 0.12),
                    transparent 34rem),
                #0b1220;
        }

        button
        {
            font: inherit;
        }

        .dashboard
        {
            width: min(100%, 940px);
            margin: 0 auto;
            animation: appear 500ms ease-out both;
        }

        .topbar,
        .brand,
        .connection-state,
        .welcome,
        .section-heading,
        .card-heading,
        .metric-reading,
        .card-footnote,
        .led-controls,
        .page-footer
        {
            display: flex;
            align-items: center;
        }

        .topbar,
        .welcome,
        .section-heading,
        .card-heading,
        .page-footer
        {
            justify-content: space-between;
        }

        .topbar
        {
            margin-bottom: 62px;
        }

        .brand
        {
            gap: 12px;
            color: inherit;
            text-decoration: none;
        }

        .brand-mark
        {
            display: grid;
            width: 42px;
            height: 42px;
            place-items: center;
            border: 1px solid rgba(112, 225, 193, 0.38);
            border-radius: 14px;
            color: var(--accent);
            background: rgba(112, 225, 193, 0.1);
            box-shadow: 0 0 28px rgba(112, 225, 193, 0.11);
        }

        .brand-mark svg
        {
            width: 23px;
            height: 23px;
        }

        .brand-name,
        .brand-caption
        {
            display: block;
        }

        .brand-name
        {
            font-size: 15px;
            font-weight: 750;
            letter-spacing: 0.02em;
        }

        .brand-caption,
        .eyebrow,
        .metric-label,
        .refresh-label,
        .page-footer
        {
            color: var(--muted);
            font-size: 11px;
            font-weight: 700;
            letter-spacing: 0.13em;
            text-transform: uppercase;
        }

        .brand-caption
        {
            margin-top: 3px;
            font-size: 9px;
        }

        .connection-state
        {
            gap: 9px;
            padding: 10px 14px;
            border: 1px solid var(--panel-border);
            border-radius: 999px;
            color: #c7d4e7;
            background: rgba(18, 29, 48, 0.62);
            font-size: 12px;
        }

        .connection-dot,
        .sensor-dot
        {
            width: 8px;
            height: 8px;
            flex: 0 0 auto;
            border-radius: 50%;
            background: var(--accent);
            box-shadow: 0 0 12px rgba(112, 225, 193, 0.68);
        }

        .connection-dot.is-offline
        {
            background: #ff8291;
            box-shadow: 0 0 12px rgba(255, 130, 145, 0.58);
        }

        .welcome
        {
            gap: 24px;
            margin-bottom: 42px;
        }

        .eyebrow
        {
            margin: 0 0 12px;
            color: var(--accent);
        }

        h1,
        h2,
        p
        {
            margin-top: 0;
        }

        h1
        {
            margin-bottom: 10px;
            font-size: clamp(32px, 6vw, 48px);
            font-weight: 720;
            letter-spacing: -0.045em;
            line-height: 1.08;
        }

        .welcome-description
        {
            margin-bottom: 0;
            color: var(--muted);
            font-size: 15px;
            line-height: 1.6;
        }

        .refresh-card
        {
            min-width: 158px;
            padding: 16px 19px;
            border: 1px solid var(--panel-border);
            border-radius: 16px;
            background: rgba(18, 29, 48, 0.58);
        }

        .refresh-label,
        .refresh-value
        {
            display: block;
        }

        .refresh-label
        {
            margin-bottom: 8px;
            font-size: 9px;
        }

        .refresh-value
        {
            font-size: 17px;
            font-weight: 650;
        }

        .section-heading
        {
            margin-bottom: 16px;
        }

        .section-heading h2
        {
            margin-bottom: 0;
            font-size: 17px;
            font-weight: 650;
        }

        .section-caption
        {
            margin-bottom: 0;
            color: var(--muted);
            font-size: 12px;
        }

        .sensor-grid
        {
            display: grid;
            grid-template-columns: repeat(2, minmax(0, 1fr));
            gap: 16px;
        }

        .panel
        {
            border: 1px solid var(--panel-border);
            border-radius: 22px;
            background:
                linear-gradient(
                    145deg,
                    rgba(25, 39, 62, 0.94),
                    var(--panel));
            box-shadow: 0 18px 48px rgba(0, 0, 0, 0.16);
        }

        .metric-card
        {
            min-height: 226px;
            padding: 24px;
        }

        .card-heading
        {
            margin-bottom: 26px;
        }

        .metric-label
        {
            margin-bottom: 0;
            font-size: 10px;
        }

        .metric-icon
        {
            display: grid;
            width: 42px;
            height: 42px;
            place-items: center;
            border-radius: 14px;
            font-size: 20px;
        }

        .metric-icon svg
        {
            width: 20px;
            height: 20px;
        }

        .temperature-icon
        {
            color: #ffae83;
            background: rgba(255, 157, 111, 0.12);
        }

        .humidity-icon
        {
            color: #8bc5ff;
            background: rgba(121, 168, 255, 0.13);
        }

        .metric-reading
        {
            align-items: baseline;
            gap: 8px;
            min-height: 70px;
        }

        .metric-value
        {
            font-size: clamp(42px, 8vw, 58px);
            font-weight: 650;
            letter-spacing: -0.055em;
            line-height: 1;
            font-variant-numeric: tabular-nums;
        }

        .metric-unit
        {
            color: var(--muted);
            font-size: 16px;
        }

        .card-footnote
        {
            gap: 9px;
            margin-top: 21px;
            color: #aab8cd;
            font-size: 12px;
        }

        .sensor-dot
        {
            width: 7px;
            height: 7px;
        }

        .control-section
        {
            margin-top: 34px;
        }

        .control-card
        {
            display: grid;
            grid-template-columns: minmax(0, 1fr) auto;
            align-items: center;
            gap: 24px;
            padding: 24px;
        }

        .control-copy h2
        {
            margin-bottom: 7px;
            font-size: 17px;
            font-weight: 650;
        }

        .control-copy p
        {
            margin-bottom: 0;
            color: var(--muted);
            font-size: 13px;
            line-height: 1.5;
        }

        .led-status
        {
            display: flex;
            align-items: center;
            gap: 13px;
            min-width: 120px;
            padding: 13px 15px;
            border: 1px solid var(--panel-border);
            border-radius: 16px;
            background: rgba(8, 14, 25, 0.38);
        }

        .led-indicator
        {
            width: 13px;
            height: 13px;
            border: 2px solid #53627a;
            border-radius: 50%;
            background: #263248;
            transition:
                background 180ms ease,
                box-shadow 180ms ease,
                border-color 180ms ease;
        }

        .led-indicator.is-on
        {
            border-color: #ffe69a;
            background: #ffd76f;
            box-shadow: 0 0 17px rgba(255, 215, 111, 0.85);
        }

        .led-status-label
        {
            display: block;
            margin-bottom: 3px;
            color: var(--muted);
            font-size: 9px;
            font-weight: 700;
            letter-spacing: 0.12em;
            text-transform: uppercase;
        }

        .led-state
        {
            font-size: 13px;
            font-weight: 650;
        }

        .led-controls
        {
            grid-column: 1 / -1;
            gap: 10px;
            padding-top: 18px;
            border-top: 1px solid var(--panel-border);
        }

        .led-button
        {
            min-height: 44px;
            padding: 0 17px;
            border: 1px solid var(--panel-border);
            border-radius: 12px;
            color: #dce7f7;
            background: rgba(12, 21, 35, 0.7);
            font-size: 13px;
            font-weight: 650;
            cursor: pointer;
            transition:
                transform 150ms ease,
                border-color 150ms ease,
                background 150ms ease;
        }

        .led-button:hover
        {
            transform: translateY(-1px);
            border-color: rgba(121, 168, 255, 0.55);
        }

        .led-button.is-active
        {
            border-color: rgba(112, 225, 193, 0.5);
            color: #caffef;
            background: rgba(54, 147, 123, 0.2);
        }

        .led-button:disabled
        {
            cursor: wait;
            opacity: 0.6;
        }

        .error-message
        {
            min-height: 22px;
            margin: 14px 2px 0;
            color: #ff9aa6;
            font-size: 13px;
        }

        .page-footer
        {
            gap: 16px;
            margin-top: 26px;
            padding-top: 18px;
            border-top: 1px solid rgba(154, 176, 210, 0.1);
            font-size: 9px;
        }

        .last-updated
        {
            color: var(--muted);
            font-size: 11px;
            font-weight: 500;
            letter-spacing: normal;
            text-transform: none;
        }

        @keyframes appear
        {
            from
            {
                transform: translateY(8px);
                opacity: 0;
            }

            to
            {
                transform: translateY(0);
                opacity: 1;
            }
        }

        @media (max-width: 620px)
        {
            body
            {
                padding: 24px 16px;
            }

            .topbar
            {
                margin-bottom: 44px;
            }

            .connection-state
            {
                padding: 9px 11px;
                font-size: 10px;
            }

            .welcome
            {
                align-items: flex-start;
                flex-direction: column;
                gap: 18px;
                margin-bottom: 32px;
            }

            .refresh-card
            {
                display: flex;
                width: 100%;
                align-items: center;
                justify-content: space-between;
                padding: 13px 16px;
            }

            .refresh-label
            {
                margin-bottom: 0;
            }

            .sensor-grid
            {
                gap: 11px;
            }

            .metric-card
            {
                min-height: 195px;
                padding: 17px 15px;
            }

            .card-heading
            {
                margin-bottom: 24px;
            }

            .metric-label
            {
                font-size: 9px;
                letter-spacing: 0.08em;
            }

            .metric-icon
            {
                width: 36px;
                height: 36px;
                border-radius: 12px;
                font-size: 17px;
            }

            .metric-reading
            {
                gap: 5px;
            }

            .metric-value
            {
                font-size: clamp(36px, 11vw, 48px);
            }

            .metric-unit
            {
                font-size: 13px;
            }

            .card-footnote
            {
                gap: 6px;
                margin-top: 18px;
                font-size: 10px;
            }

            .control-section
            {
                margin-top: 27px;
            }

            .control-card
            {
                grid-template-columns: minmax(0, 1fr) auto;
                gap: 15px;
                padding: 19px 16px;
            }

            .led-status
            {
                min-width: 98px;
                gap: 9px;
                padding: 11px 10px;
            }

            .led-controls
            {
                gap: 8px;
            }

            .led-button
            {
                flex: 1;
                padding: 0 9px;
                font-size: 12px;
            }
        }

        @media (prefers-reduced-motion: reduce)
        {
            *,
            *::before,
            *::after
            {
                scroll-behavior: auto !important;
                animation-duration: 0.01ms !important;
                transition-duration: 0.01ms !important;
            }
        }
    </style>
</head>
<body>
    <main class="dashboard">
        <header class="topbar">
            <a class="brand" href="/" aria-label="YoloUNO trang chủ">
                <span class="brand-mark" aria-hidden="true">
                    <svg viewBox="0 0 24 24" fill="none">
                        <path
                            d="M12 3v3m0 12v3M3 12h3m12 0h3M5.64 5.64l2.12 2.12m8.48 8.48 2.12 2.12m0-12.72-2.12 2.12m-8.48 8.48-2.12 2.12"
                            stroke="currentColor"
                            stroke-width="1.7"
                            stroke-linecap="round"/>
                        <circle
                            cx="12"
                            cy="12"
                            r="4.3"
                            stroke="currentColor"
                            stroke-width="1.7"/>
                    </svg>
                </span>
                <span>
                    <span class="brand-name">YoloUNO</span>
                    <span class="brand-caption">Smart device dashboard</span>
                </span>
            </a>

            <div class="connection-state" role="status">
                <span
                    class="connection-dot"
                    id="connectionDot"
                    aria-hidden="true"></span>
                <span id="connectionLabel">Đang kết nối thiết bị</span>
            </div>
        </header>

        <section class="welcome" aria-labelledby="pageTitle">
            <div>
                <p class="eyebrow">Bảng điều khiển thiết bị</p>
                <h1 id="pageTitle">Xin chào, YoloUNO.</h1>
                <p class="welcome-description">
                    Theo dõi môi trường và điều khiển thiết bị của bạn.
                </p>
            </div>

            <div class="refresh-card">
                <span class="refresh-label">Chu kỳ cập nhật</span>
                <strong class="refresh-value">Mỗi 2 giây</strong>
            </div>
        </section>

        <section aria-labelledby="overviewTitle">
            <div class="section-heading">
                <h2 id="overviewTitle">Tổng quan cảm biến</h2>
                <p class="section-caption">Dữ liệu trực tiếp từ DHT20</p>
            </div>

            <div class="sensor-grid">
                <article class="panel metric-card">
                    <div class="card-heading">
                        <p class="metric-label">Nhiệt độ</p>
                        <span
                            class="metric-icon temperature-icon"
                            aria-hidden="true">
                            <svg viewBox="0 0 24 24" fill="none">
                                <path
                                    d="M14 14.76V5a3 3 0 0 0-6 0v9.76a5 5 0 1 0 6 0Z"
                                    stroke="currentColor"
                                    stroke-width="1.8"
                                    stroke-linecap="round"
                                    stroke-linejoin="round"/>
                                <path
                                    d="M11 12v5"
                                    stroke="currentColor"
                                    stroke-width="1.8"
                                    stroke-linecap="round"/>
                            </svg>
                        </span>
                    </div>
                    <div class="metric-reading">
                        <strong class="metric-value" id="temperature">--</strong>
                        <span class="metric-unit">°C</span>
                    </div>
                    <div class="card-footnote">
                        <span class="sensor-dot" aria-hidden="true"></span>
                        <span id="sensorStatus">Đang chờ dữ liệu cảm biến</span>
                    </div>
                </article>

                <article class="panel metric-card">
                    <div class="card-heading">
                        <p class="metric-label">Độ ẩm không khí</p>
                        <span
                            class="metric-icon humidity-icon"
                            aria-hidden="true">
                            <svg viewBox="0 0 24 24" fill="none">
                                <path
                                    d="M12 3.5s-6.5 7.1-6.5 11.2a6.5 6.5 0 0 0 13 0C18.5 10.6 12 3.5 12 3.5Z"
                                    stroke="currentColor"
                                    stroke-width="1.8"
                                    stroke-linecap="round"
                                    stroke-linejoin="round"/>
                                <path
                                    d="M9 15.5c0 1.7 1.3 3 3 3"
                                    stroke="currentColor"
                                    stroke-width="1.8"
                                    stroke-linecap="round"/>
                            </svg>
                        </span>
                    </div>
                    <div class="metric-reading">
                        <strong class="metric-value" id="humidity">--</strong>
                        <span class="metric-unit">%</span>
                    </div>
                    <div class="card-footnote">
                        <span class="sensor-dot" aria-hidden="true"></span>
                        <span>Dữ liệu cập nhật tự động</span>
                    </div>
                </article>
            </div>
        </section>

        <section class="control-section" aria-labelledby="controlTitle">
            <div class="section-heading">
                <h2 id="controlTitle">Điều khiển thiết bị</h2>
                <p class="section-caption">Thao tác trong mạng nội bộ</p>
            </div>

            <div class="panel control-card">
                <div class="control-copy">
                    <h2>Đèn LED tích hợp</h2>
                    <p>Bật hoặc tắt đèn LED trên board YoloUNO.</p>
                </div>

                <div class="led-status">
                    <span
                        class="led-indicator"
                        id="ledIndicator"
                        aria-hidden="true"></span>
                    <span>
                        <span class="led-status-label">Trạng thái</span>
                        <strong class="led-state" id="ledState">Đang tắt</strong>
                    </span>
                </div>

                <div class="led-controls">
                    <button
                        class="led-button"
                        id="turnLedOnButton"
                        type="button"
                        onclick="setLedState('on')">
                        Bật đèn
                    </button>
                    <button
                        class="led-button"
                        id="turnLedOffButton"
                        type="button"
                        onclick="setLedState('off')">
                        Tắt đèn
                    </button>
                </div>
            </div>

            <p class="error-message" id="errorMessage" role="status"></p>
        </section>

        <footer class="page-footer">
            <span>YoloUNO · IoT local dashboard</span>
            <span class="last-updated" id="lastUpdated">
                Đang chờ lần cập nhật đầu tiên
            </span>
        </footer>
    </main>

    <script>
        async function updateSensorAndLedInformation()
        {
            const errorMessage = document.getElementById('errorMessage');
            const connectionDot = document.getElementById('connectionDot');
            const connectionLabel =
                document.getElementById('connectionLabel');
            const sensorStatus = document.getElementById('sensorStatus');
            const lastUpdated = document.getElementById('lastUpdated');

            try
            {
                const response = await fetch(
                    '/data',
                    {
                        cache: 'no-store'
                    });

                if (!response.ok)
                {
                    throw new Error('Không thể tải dữ liệu từ YoloUNO.');
                }

                const deviceData = await response.json();
                const temperatureElement =
                    document.getElementById('temperature');
                const humidityElement =
                    document.getElementById('humidity');
                const ledStateElement =
                    document.getElementById('ledState');
                const ledIndicator =
                    document.getElementById('ledIndicator');
                const turnLedOnButton =
                    document.getElementById('turnLedOnButton');
                const turnLedOffButton =
                    document.getElementById('turnLedOffButton');

                temperatureElement.textContent =
                    deviceData.temperature === null
                        ? '--'
                        : deviceData.temperature.toFixed(1);

                humidityElement.textContent =
                    deviceData.humidity === null
                        ? '--'
                        : deviceData.humidity.toFixed(1);

                ledStateElement.textContent =
                    deviceData.led ? 'Đang bật' : 'Đang tắt';
                ledIndicator.classList.toggle('is-on', deviceData.led);
                turnLedOnButton.classList.toggle('is-active', deviceData.led);
                turnLedOffButton.classList.toggle('is-active', !deviceData.led);
                connectionDot.classList.remove('is-offline');
                connectionLabel.textContent = 'Đang hoạt động';
                sensorStatus.textContent =
                    deviceData.temperature === null
                        ? 'Không nhận được dữ liệu'
                        : 'Cảm biến đang hoạt động';
                lastUpdated.textContent =
                    'Cập nhật lúc ' +
                    new Date().toLocaleTimeString('vi-VN');

                errorMessage.textContent =
                    deviceData.temperature === null
                        ? 'Không đọc được cảm biến DHT20. Hãy kiểm tra kết nối.'
                        : '';
            }
            catch (error)
            {
                connectionDot.classList.add('is-offline');
                connectionLabel.textContent = 'Mất kết nối thiết bị';
                sensorStatus.textContent = 'Không thể đọc cảm biến';
                errorMessage.textContent = error.message;
            }
        }

        async function setLedState(requestedLedState)
        {
            const errorMessage = document.getElementById('errorMessage');
            const turnLedOnButton =
                document.getElementById('turnLedOnButton');
            const turnLedOffButton =
                document.getElementById('turnLedOffButton');

            try
            {
                turnLedOnButton.disabled = true;
                turnLedOffButton.disabled = true;

                const response = await fetch(
                    '/led',
                    {
                        method: 'POST',
                        headers:
                        {
                            'Content-Type':
                                'application/x-www-form-urlencoded'
                        },
                        body:
                            'state=' +
                            encodeURIComponent(requestedLedState)
                    });

                if (!response.ok)
                {
                    const errorDetails = await response.text();
                    throw new Error(errorDetails);
                }

                errorMessage.textContent = '';
                await updateSensorAndLedInformation();
            }
            catch (error)
            {
                errorMessage.textContent = error.message;
            }
            finally
            {
                turnLedOnButton.disabled = false;
                turnLedOffButton.disabled = false;
            }
        }

        updateSensorAndLedInformation();
        window.setInterval(updateSensorAndLedInformation, 2000);
    </script>
</body>
</html>
)HTML";

void sendWebPage()
{
    webServer.sendHeader("Cache-Control", "no-store");
    webServer.send_P(
        200,
        "text/html; charset=utf-8",
        webPage);
}

void sendDeviceData()
{
    const String temperatureValue = sensorDataIsValid
        ? String(temperatureCelsius, 1)
        : "null";
    const String humidityValue = sensorDataIsValid
        ? String(relativeHumidity, 1)
        : "null";

    const String responseData =
        String("{\"temperature\":") +
        temperatureValue +
        ",\"humidity\":" +
        humidityValue +
        ",\"led\":" +
        (ledIsOn ? "true" : "false") +
        "}";

    webServer.sendHeader("Cache-Control", "no-store");
    webServer.send(200, "application/json", responseData);
}

void setLedState()
{
    if (!webServer.hasArg("state"))
    {
        webServer.send(
            400,
            "text/plain; charset=utf-8",
            "Missing required state parameter. Use on or off.");
        return;
    }

    const String requestedLedState = webServer.arg("state");

    if (requestedLedState != "on" && requestedLedState != "off")
    {
        webServer.send(
            400,
            "text/plain; charset=utf-8",
            "Invalid state parameter. Use on or off.");
        return;
    }

    ledIsOn = requestedLedState == "on";
    digitalWrite(ledPin, ledIsOn ? HIGH : LOW);

    Serial.printf(
        "LED GPIO %u: %s\n",
        static_cast<unsigned int>(ledPin),
        ledIsOn ? "ON" : "OFF");

    webServer.send(200, "text/plain; charset=utf-8", "OK");
}

void registerWebServerRoutes()
{
    webServer.on("/", HTTP_GET, sendWebPage);
    webServer.on("/data", HTTP_GET, sendDeviceData);
    webServer.on("/led", HTTP_POST, setLedState);
    webServer.begin();
    webServerIsReady = true;

    Serial.println("Web server started on port 80.");
}

void printAccessPointStatus()
{
    Serial.printf("Wi-Fi access point name: %s\n", accessPointName);
    Serial.print("Web server address: http://");
    Serial.print(WiFi.softAPIP());
    Serial.println("/");
}

void printAccessPointStatusPeriodically()
{
    if (!webServerIsReady)
    {
        return;
    }

    const unsigned long currentTimeMilliseconds = millis();

    if (currentTimeMilliseconds -
        lastAccessPointStatusPrintMilliseconds <
        accessPointStatusIntervalMilliseconds)
    {
        return;
    }

    lastAccessPointStatusPrintMilliseconds = currentTimeMilliseconds;
    printAccessPointStatus();
}

void readTemperatureAndHumidity()
{
    const unsigned long currentTimeMilliseconds = millis();

    if (currentTimeMilliseconds - lastSensorReadMilliseconds <
        sensorReadIntervalMilliseconds)
    {
        return;
    }

    lastSensorReadMilliseconds = currentTimeMilliseconds;

    const int sensorReadStatus = temperatureHumiditySensor.read();
    sensorDataIsValid = sensorReadStatus == DHT20_OK;

    if (!sensorDataIsValid)
    {
        Serial.printf(
            "DHT20 read failed with status %d.\n",
            sensorReadStatus);
        return;
    }

    temperatureCelsius = temperatureHumiditySensor.getTemperature();
    relativeHumidity = temperatureHumiditySensor.getHumidity();

    Serial.printf(
        "Temperature: %.1f C, relative humidity: %.1f%%\n",
        temperatureCelsius,
        relativeHumidity);
}

}

void setup()
{
    Serial.begin(115200);

    const unsigned long serialWaitStartedMilliseconds = millis();

    while (!Serial &&
           millis() - serialWaitStartedMilliseconds < 5000)
    {
        delay(10);
    }

    Serial.println();
    Serial.println("Starting the YoloUNO sensor web server.");

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    const bool sensorBusStarted = Wire.begin(sensorDataPin, sensorClockPin);

    if (sensorBusStarted && temperatureHumiditySensor.isConnected())
    {
        Serial.println("DHT20 sensor detected at I2C address 0x38.");
    }
    else
    {
        Serial.println(
            "DHT20 sensor not detected at I2C address 0x38. Check the wiring.");
    }

    if (!WiFi.softAP(accessPointName, accessPointPassword))
    {
        Serial.println("Error: failed to start the Wi-Fi access point.");
        return;
    }

    registerWebServerRoutes();
    printAccessPointStatus();
    lastAccessPointStatusPrintMilliseconds = millis();
}

void loop()
{
    webServer.handleClient();
    readTemperatureAndHumidity();
    printAccessPointStatusPeriodically();
    delay(webServerResponseDelayMilliseconds);
}
