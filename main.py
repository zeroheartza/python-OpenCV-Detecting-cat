
import argparse
import cv2
import requests
import time
import RPi.GPIO as GPIO
import os
import datetime, pytz
def Capture():
    
    os.system("python3 camera.py")

def Pir():
    PIR_input = 7
    n=0		
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BOARD)		
    GPIO.setup(PIR_input, GPIO.IN)	
    while True:

        i =GPIO.input(PIR_input)
        if(i==1):
            print("Intruders detected",i)
            LED_ON()
            n+=1
            if(n==3):
                print("Detect Cat",i)
                OpenCv()
        else:
            n=0
            LED_OFF()
            print("NO intruders",i)
        time.sleep(0.1)

def LED_ON():
    GPIO.setwarnings(False) 
    GPIO.setmode(GPIO.BOARD) 
    GPIO.setup(35,GPIO.OUT)
    GPIO.output(35,GPIO.HIGH)

def LED_OFF():
    GPIO.setwarnings(False) 
    GPIO.setmode(GPIO.BOARD) 
    GPIO.setup(35,GPIO.OUT)
    GPIO.output(35,GPIO.LOW)
    
def now():
    
    tz = pytz.timezone('Asia/Bangkok')
    now1 = datetime.datetime.now(tz)
    month_name = 'x มกราคม กุมภาพันธ์ มีนาคม เมษายน พฤษภาคม มิถุนายน กรกฎาคม สิงหาคม กันยายน ตุลาคม พฤศจิกายน ธันวาคม'.split()[now1.month]
    thai_year = now1.year + 543
    time_str = now1.strftime('%H:%M:%S')
    return ("%d %s %d %s"%(now1.day, month_name, thai_year, time_str))

def ServoOn():
    os.system('sudo ./vs1003')
    os.system("python3 servo3.py")
    time.sleep(10)
    
    os.system("python3 servo4.py")

def ServoOff():
    os.system("python3 servo4.py")
    
def OpenCv():
    Capture()
    ap = argparse.ArgumentParser()

    ap.add_argument("-c", "--cascade",
                default="haarcascade_frontalcatface.xml",
                help="path to cat detector haar cascade")
    args = vars(ap.parse_args())

    image = cv2.imread("img.jpg")
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)


    detector = cv2.CascadeClassifier(args["cascade"])
    rects = detector.detectMultiScale(gray, scaleFactor=1.3,
                minNeighbors=10, minSize=(75, 75))

    if(len(rects) != 0):
        print("Detect Success")
        for (i, (x, y, w, h)) in enumerate(rects):
            cv2.rectangle(image, (x, y), (x + w, y + h), (0, 0, 255), 2)
            cv2.putText(image, "Cat #{}".format(i + 1), (x, y - 10),
            cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 0, 255), 2)

        cv2.imwrite('result.jpg', image)


        headers = {
                "Authorization":"Bearer T18gHDqcNqdXluWcnuEeCfN6HF7aNfdJG9RFVaJzjM0"
            }
        data = {
                
                "message": now()
            }
        files = {
                "imageFile": open("result.jpg","rb")
            }
        requests.post("https://notify-api.line.me/api/notify", headers=headers, data=data, files=files)
        ServoOn()
            
    else:
        ServoOff()

    LED_OFF()
        


def run():
    LED_OFF()
    Pir()     
    time.sleep(2)



run()

