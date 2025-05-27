#include <MPU9250_asukiaaa.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>

#ifdef _ESP32_HAL_I2C_H_
#define SDA_PIN 21
#define SCL_PIN 22
#endif

#define BUZZER_PIN 2

// WiFi credentials
const char* ssid = "naim";
const char* password = "naimz123";

WebServer server(80);
MPU9250_asukiaaa mySensor;

// Calibration constants
const int MOVING_AVG_SIZE = 15;
const float baselineAngle = -96.4;
const float ANGLE_CORRECTION = 0.952;
const float ZERO_OFFSET = 2.8;
const float STABILITY_THRESHOLD = 0.75;
const float ANGLE_THRESHOLD = 60.0;

// Variables for moving average
float angleBuffer[MOVING_AVG_SIZE];
int bufferIndex = 0;
bool bufferFilled = false;
float currentAngle = 0;

// Timer variables
unsigned long startTime = 0;
bool measurementStarted = false;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>Trunk Flexion Monitor</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.9.1/chart.min.js"></script>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            text-align: center; 
            margin: 0;
            padding: 20px;
            background-color: #f0f0f0;
        }
        .card {
            background-color: white;
            border-radius: 10px;
            padding: 20px;
            margin: 10px auto;
            max-width: 800px;
            box-shadow: 0 2px 5px rgba(0,0,0,0.1);
        }
        h1 {
            margin: 0;
            padding: 10px;
            font-size: 32px;
        }
        .angle {
            font-size: 120px;
            font-weight: bold;
            margin: 20px 0;
            line-height: 1;
        }
        .status {
            font-size: 36px;
            padding: 15px;
            border-radius: 5px;
            margin: 15px 0;
            text-align: center;
            width: 100%;
            box-sizing: border-box;
            transition: all 0.3s ease;
        }
        .good { background-color: #4CAF50; color: white; }
        .bad { background-color: #f44336; color: white; }
        #startBtn {
            font-size: 24px;
            padding: 10px 20px;
            margin: 10px;
            cursor: pointer;
            border: 2px solid #333;
            background-color: white;
            border-radius: 5px;
            transition: all 0.3s ease;
        }
        #startBtn:hover {
            background-color: #333;
            color: white;
        }
        .chart-container {
            height: 400px;
            width: 100%;
            margin-top: 20px;
            position: relative;
        }
    </style>
</head>
<body>
    <div class="card">
        <h1>Trunk Flexion Monitor</h1>
        <div id="angleValue" class="angle">0.0°</div>
        <div id="status" class="status good">GOOD POSTURE</div>
        <button id="startBtn" onclick="startMeasurement()">Start New Measurement</button>
    </div>
    <div class="card">
        <div class="chart-container">
            <canvas id="angleChart"></canvas>
        </div>
    </div>

    <script>
        let chart;
        let isRecording = false;
        let startTime;

        // Line chart configuration
        const config = {
            type: 'line',
            data: {
                datasets: [{
                    label: 'Spine Angle',
                    data: [],
                    borderWidth: 2,
                    tension: 0.4,
                    pointRadius: 0,
                    borderColor: function(context) {
                        if (!context.raw) return 'rgb(75, 192, 192)';
                        return context.raw.y > 60 ? 'rgb(255, 99, 132)' : 'rgb(75, 192, 192)';
                    },
                    segment: {
                        borderColor: ctx => ctx.p1.parsed.y > 60 ? 'rgb(255, 99, 132)' : 'rgb(75, 192, 192)'
                    }
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                animation: false,
                plugins: {
                    title: {
                        display: true,
                        text: 'Angle of spine',
                        font: { size: 20 },
                        padding: 20
                    }
                },
                scales: {
                    x: {
                        type: 'linear',
                        title: {
                            display: true,
                            text: 'Time (ms)'
                        },
                        min: 0,
                        max: 10000,
                        ticks: {
                            callback: function(value) {
                                return (value/1000).toFixed(0) + 's';
                            }
                        }
                    },
                    y: {
                        title: {
                            display: true,
                            text: 'Angle (°)'
                        },
                        min: 0,
                        max: 90
                    }
                }
            }
        };

        function initChart() {
            const ctx = document.getElementById('angleChart').getContext('2d');
            chart = new Chart(ctx, config);
        }

        function updateData() {
            if (!isRecording) return;

            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    const currentTime = Date.now() - startTime;
                    
                    // Update angle display and status
                    document.getElementById("angleValue").innerHTML = data.angle.toFixed(1) + "°";
                    const status = document.getElementById("status");
                    if(data.angle > 60) {
                        status.innerHTML = "BAD POSTURE";
                        status.className = "status bad";
                    } else {
                        status.innerHTML = "GOOD POSTURE";
                        status.className = "status good";
                    }

                    // Add new data point
                    chart.data.datasets[0].data.push({
                        x: currentTime,
                        y: data.angle
                    });

                    // Update x-axis scale if needed
                    const maxX = Math.max(10000, currentTime);
                    chart.options.scales.x.max = maxX;
                    chart.options.scales.x.min = Math.max(0, maxX - 10000);

                    chart.update('none');
                })
                .catch(error => console.error('Error:', error));

            setTimeout(updateData, 100);
        }

        function startMeasurement() {
            isRecording = !isRecording;
            const btn = document.getElementById('startBtn');
            
            if (isRecording) {
                startTime = Date.now();
                chart.data.datasets[0].data = [];
                chart.options.scales.x.min = 0;
                chart.options.scales.x.max = 10000;
                btn.textContent = 'Stop Measurement';
                btn.style.backgroundColor = '#f44336';
                btn.style.color = 'white';
                btn.style.borderColor = '#f44336';
                updateData();
            } else {
                btn.textContent = 'Start New Measurement';
                btn.style.backgroundColor = 'white';
                btn.style.color = 'black';
                btn.style.borderColor = '#333';
                // Keep the last data visible
                chart.update();
            }
            
            fetch('/start');
        }

        window.onload = function() {
            initChart();
        };
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);
    
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    mySensor.setWire(&Wire);
    mySensor.beginAccel();
    
    for(int i = 0; i < MOVING_AVG_SIZE; i++) {
        angleBuffer[i] = 0;
    }

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", index_html);
    });

    server.on("/data", HTTP_GET, []() {
        String json = "{\"angle\":" + String(currentAngle, 2) + "}";
        server.send(200, "application/json", json);
    });

    server.on("/start", HTTP_GET, []() {
        startTime = millis();
        measurementStarted = true;
        server.send(200, "text/plain", "Measurement started");
    });

    server.begin();
}

float calculateAngle() {
    mySensor.accelUpdate();
    
    float accelY = mySensor.accelY();
    float accelZ = mySensor.accelZ();
    
    float rawAngle = atan2(accelY, accelZ) * RAD_TO_DEG;
    float baselineAdjusted = (rawAngle - baselineAngle) * ANGLE_CORRECTION;
    float calibratedAngle = baselineAdjusted - ZERO_OFFSET;
    
    if (calibratedAngle < -1.0) calibratedAngle = 0.0;
    if (calibratedAngle > 90.0) calibratedAngle = 90.0;
    
    angleBuffer[bufferIndex] = calibratedAngle;
    bufferIndex = (bufferIndex + 1) % MOVING_AVG_SIZE;
    if (bufferIndex == 0) bufferFilled = true;
    
    float sum = 0;
    int count = bufferFilled ? MOVING_AVG_SIZE : bufferIndex;
    for(int i = 0; i < count; i++) {
        sum += angleBuffer[i];
    }
    float avgAngle = sum / count;
    
    digitalWrite(BUZZER_PIN, avgAngle > ANGLE_THRESHOLD ? HIGH : LOW);
    
    return avgAngle;
}

void loop() {
    currentAngle = calculateAngle();
    server.handleClient();
    delay(10);
}