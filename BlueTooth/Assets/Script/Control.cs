using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using ArduinoBluetoothAPI;
using System;
using System.Text;
using System.IO;
using UnityEngine.UI;
using TMPro;

public class Control : MonoBehaviour
{

    // Use this for initialization
    BluetoothHelper bluetoothHelper;
    string deviceName;
    string received_message;
    private string tmp;
   

    
   
   // public Text State;
    public TMP_Text flSpeed;
    public TMP_Text angle;
    public TMP_Text timeCount;
   // public Text Angle;
    [SerializeField]
    private int fL_count = 200;
    [SerializeField]
    private int pl_count = 360;
    public int pl_module = 10;
    [SerializeField]
    private float rpm_count = 2.4f;
    [SerializeField]
    private float rpm_time = 25;
    public Image on;
    public Image off;
    public Image connect;
    public Image disconnect;
    public Image start;
    public Image stop;
    [SerializeField]
    private bool state = false;
    [SerializeField]
    private float time=25f;
    [SerializeField]
    private float r_Time = 25f;
    public Text Test;

    public static Control instance;   //변수 선언부// 

    void Awake()
    {

        Control.instance = this;  //변수 초기화부 // 

    }
    void Start()
    {


        deviceName = "UART Service"; //bluetooth should be turned ON;
        try
        {
            BluetoothHelper.BLE = true;
            bluetoothHelper = BluetoothHelper.GetInstance(deviceName);
            bluetoothHelper.OnConnected += OnConnected;
            bluetoothHelper.OnConnectionFailed += OnConnectionFailed;
            bluetoothHelper.OnDataReceived += OnMessageReceived; //read the data
            bluetoothHelper.OnScanEnded += OnScanEnded;

            bluetoothHelper.setTerminatorBasedStream("\n");

            if (!bluetoothHelper.ScanNearbyDevices())
            {
                //scan didnt start (on windows desktop (not UWP))
                //try to connect
                bluetoothHelper.Connect();//this will work only for bluetooth classic.
                //scanning is mandatory before connecting for BLE.

            }
        }
        catch (Exception ex)
        {
           // Debug.Log(ex.Message);
           
            write(ex.Message);
          
        }
    }
    
    private void write(string msg)
    {
        tmp += ">" + msg + "\n";
    }

    void OnMessageReceived(BluetoothHelper helper)
    {
        received_message = helper.Read();
       // Debug.Log(received_message);
       
        write("Received : " + received_message);
       
    }

    void OnConnected(BluetoothHelper helper)
    {
        try
        {
            helper.StartListening();
        }
        catch (Exception ex)
        {
           // Debug.Log(ex.Message);
           
            write(ex.Message);
           

        }
    }

    void OnScanEnded(BluetoothHelper helper, LinkedList<BluetoothDevice> devices)
    {

        if (helper.isDevicePaired()) //we did found our device (with BLE) or we already paired the device (for Bluetooth Classic)
        {
            helper.Connect();
            connect.gameObject.SetActive(true);
            disconnect.gameObject.SetActive(false);
          //  Debug.Log("Connect");
        }

        else
            helper.ScanNearbyDevices(); //we didn't
    }

    void OnConnectionFailed(BluetoothHelper helper)
    {
        write("Connection Failed");
        connect.gameObject.SetActive(false);
        disconnect.gameObject.SetActive(true);
      //  Debug.Log("Connection Failed");
    }


    //Call this function to emulate message receiving from bluetooth while debugging on your PC.
    public void ButtonNumber(int num)
    {


        if (bluetoothHelper.isConnected())
        {
           // Test.text = bluetoothHelper.Read();

            if (num == 0)
            {
                state = false;
                bluetoothHelper.SendData("0");
               
             //   Debug.Log("Stop");
              
                on.gameObject.SetActive(false);
                off.gameObject.SetActive(true);
              
            }

            if (num == 1)
            {
                state = true;
                bluetoothHelper.SendData("1");
             //   Debug.Log("Start");
             //   Debug.Log(time);
                on.gameObject.SetActive(true);
                off.gameObject.SetActive(false);
            }

            if (num == 2)
            {
                bluetoothHelper.SendData("2");
              //  Debug.Log("Reset");
                //State.text = "Reset";
                fL_count = 200;
                pl_count = 360;
                rpm_count = 2.4f;
                rpm_time = 25f;
                time = 25f;
                r_Time = 25;
                angle.text = pl_count.ToString()+"°";

                //   pl_count = 248640;

                flSpeed.text = rpm_count.ToString();
               


              //  Angle.text = " ";

            }
            if (num == 3)
            {
                bluetoothHelper.SendData("3");
              //  Debug.Log("fL_count up");
              
                fL_count = fL_count - 10;
                if (fL_count < 150)
                {
                    fL_count = 0;
                }

              r_Time -= 1.0f;
                if (r_Time < 19)
                {
                    r_Time = 19;
                }
                rpm_time = r_Time;
                time = rpm_time;
                rpm_count =Mathf.Round(((360 / rpm_time) / 6)*100)/100 ; //1바퀴= 360도 , 1초 =360/time , 60초= (360/time)*60 , RPM = (360/time)*60/360 
               // Debug.Log(rpm_time);
                flSpeed.text = rpm_count.ToString();
               
            }
            if (num == 4)
            {
                bluetoothHelper.SendData("4");
               // Debug.Log("fL_count down");
                fL_count = fL_count + 10;
                if (fL_count > 400)
                {
                    fL_count = 400;
                }
                r_Time += 1.0f;
                if (r_Time > 50)
                {
                    r_Time = 50;
                }
                rpm_time = r_Time;
                time = rpm_time;
              //  Debug.Log(rpm_time);
                rpm_count = Mathf.Round(((360 / rpm_time) / 6) * 100) / 100; //1바퀴= 360도 , 1초 =360/time , 60초= (360/time)*60 , RPM = (360/time)*60/360 
                flSpeed.text = rpm_count.ToString();
                
            }
            if (num == 5)
            {
                bluetoothHelper.SendData("5");
             //   Debug.Log("ac_count up");
               
            }
            if (num == 6)
            {
                bluetoothHelper.SendData("6");
            //    Debug.Log("ac_count down");
               
            }
            if (num == 7)
            {
                bluetoothHelper.SendData("7");
              //  Debug.Log("pl_count up");
                pl_count = pl_count + pl_module;
                r_Time += 0.65f;
                if (pl_count > 720)
                {
                    pl_count = 720;
                    r_Time = 50;
                }
               
               
                rpm_time = r_Time;
                time = rpm_time;
             //   Debug.Log(rpm_time);
                angle.text = pl_count.ToString() + "°";
            }
            if (num == 8)
            {
                bluetoothHelper.SendData("8");
              //  Debug.Log("pl_count down");
                pl_count = pl_count - pl_module;
                r_Time -= 0.65f;
                if (pl_count < 0)
                {
                    pl_count = 0;
                    r_Time = 0;
                }
               
                rpm_time = r_Time;
                time = rpm_time;
              // Debug.Log(rpm_time);
                angle.text =  pl_count.ToString()+ "°";
            }
           

        }

    }
    private void Update()
    {
       
        if (state == true)
        {
            time -= Time.deltaTime;
            timeCount.text = time.ToString();
            timeCount.text = string.Format("{0:N1}", time);
            
            if (time < 0)
            {
                ButtonNumber(0);
                time =rpm_time;
                timeCount.text = "00.0";
                state =false;
            }
        }
        else
        {
            timeCount.text = "00.0";
            time =rpm_time;
        }
    }
    void OnDestroy()
    {
        if (bluetoothHelper != null)
            bluetoothHelper.Disconnect();


    }
  
    public void HomeButtonClick()
    {
        Application.OpenURL("http://www.tsp-xr.com/");
    }
    public void YoutubeButtonClick()
    {
        Application.OpenURL("https://www.youtube.com/channel/UCLwUWEbxXnapxa5xh3ewgEQ/");
    }
    /*  private void OnApplicationQuit()
      {
          Control.instance.ButtonLed(0);
          Debug.Log("END");
      }*/
}