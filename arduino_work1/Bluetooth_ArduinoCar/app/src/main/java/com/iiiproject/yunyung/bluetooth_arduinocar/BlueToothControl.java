package com.iiiproject.yunyung.bluetooth_arduinocar;


import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.speech.RecognitionListener;
import android.speech.RecognizerIntent;
import android.speech.SpeechRecognizer;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import org.w3c.dom.Text;

import java.io.IOException;
import java.util.List;
import java.util.UUID;

public class BlueToothControl extends AppCompatActivity {
    String address;
    private SpeechRecognizer recognizer;
    Intent intent;
    TextView txt_SpeechResult;
    Button btn_forward, btn_backward, btn_left, btn_right, btn_stop, btn_rectangle, btn_triangle, btn_openSpeech, btnDis;
    BluetoothAdapter myBluetooth = null;
    BluetoothSocket btSocket = null;
    private boolean isBtConnected = false;
    static final UUID myUUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    ConnectBT mConnectBT = new ConnectBT();
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        super.setContentView(R.layout.bluetooth_control);
        //receive the address of the bluetooth device
        Intent i = this.getIntent();
        address = i.getStringExtra("EXTRA_ADDRESS");
        mConnectBT.execute();
        txt_SpeechResult = (TextView) findViewById(R.id.txt_SpeechResult);
        btn_openSpeech = (Button) findViewById(R.id.btn_openSpeech);
        btnDis = (Button)findViewById(R.id.btnDis);
        btn_forward = (Button)findViewById(R.id.btn_forward);
        btn_backward = (Button)findViewById(R.id.btn_backward);
        btn_left = (Button)findViewById(R.id.btn_left);
        btn_right = (Button)findViewById(R.id.btn_right);
        btn_stop = (Button)findViewById(R.id.btn_stop);
        btn_rectangle = (Button)findViewById(R.id.btn_rectangle);
        btn_triangle = (Button)findViewById(R.id.btn_triangle);
        btnDis.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                Disconnect(); //close connection
            }
        });

        btn_forward.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                sendMessageToDevice("W");
            }
        });

        btn_backward.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                sendMessageToDevice("S");
            }
        });

        btn_left.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                sendMessageToDevice("A");
            }
        });

        btn_right.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                sendMessageToDevice("D");
            }
        });

        btn_stop.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                sendMessageToDevice("Q");
            }
        });

        btn_rectangle.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                sendMessageToDevice("I");
            }
        });

        btn_triangle.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                sendMessageToDevice("P");
            }
        });


        //按 Button 時，呼叫 SpeechRecognizer 的 startListening()
        //Intent 為傳遞給 SpeechRecognizer 的參數
        btn_openSpeech.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                if (btn_openSpeech.getText().equals("開啟語音辨識"))
                {
                    recognizer = SpeechRecognizer.createSpeechRecognizer(getApplicationContext());
                    recognizer.setRecognitionListener(new MyRecognizerListener());
                    intent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
                    intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);
                    recognizer.startListening(intent);
                    btn_openSpeech.setText("語音辨識中...再次點擊可關閉");
                    txt_SpeechResult.setText("請對麥克風輸入指令");
                }
                else{
                    recognizer.cancel();
                    recognizer.stopListening();
                    recognizer.destroy();
                    txt_SpeechResult.setText("語音結果輸出區塊");
                    btn_openSpeech.setText("開啟語音辨識");
                }
            }
        });
    }

    private class MyRecognizerListener implements RecognitionListener {

        @Override
        public void onResults(Bundle results) {
            List<String> resList = results.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
            try{
                for(String res: resList) {
                    if (res.equals("前進") || res.contains("前")){
                        txt_SpeechResult.setText("前進!");
                        sendMessageToDevice("W");
                        break;
                    }
                    else if (res.equals("後退") || res.contains("後") || res.contains("退")){
                        txt_SpeechResult.setText("後退!");
                        sendMessageToDevice("S");
                        break;
                    }
                    else if (res.equals("左轉") || res.contains("左")){
                        txt_SpeechResult.setText("左轉!");
                        sendMessageToDevice("A");
                        break;
                    }
                    else if (res.equals("右轉") || res.contains("右")){
                        txt_SpeechResult.setText("右轉!");
                        sendMessageToDevice("D");
                        break;
                    }
                    else if (res.equals("停止") || res.contains("停") || res.contains("止")){
                        txt_SpeechResult.setText("停止!");
                        sendMessageToDevice("Q");
                        break;
                    }
                    else if (res.equals("繞正方形") || res.contains("正")){
                        txt_SpeechResult.setText("繞正方形!");
                        sendMessageToDevice("I");
                        break;
                    }
                    else if (res.equals("繞三角形") || res.contains("三") || res.contains("角")){
                        txt_SpeechResult.setText("繞三角形!");
                        sendMessageToDevice("P");
                        break;
                    }
                    else{
                        txt_SpeechResult.setText("查無此指令! 偵測到指令為: " + resList.get(0));
                    }
                }
            }
            catch(Exception e){
                Log.d("NULLLL", "NULLLL");
            }

        }

        @Override
        public void onError(int error) {
            Log.d("RECOGNIZER", "Error Code: " + error);
            if(error > 0) {
                recognizer.startListening(intent);
            }
        }

        @Override
        public void onReadyForSpeech(Bundle params) {
            Log.d("RECOGNIZER", "ready");
        }

        @Override
        public void onBeginningOfSpeech() {
            Log.d("RECOGNIZER", "beginning");
        }

        @Override
        public void onRmsChanged(float rmsdB) {
        }

        @Override
        public void onBufferReceived(byte[] buffer) {
            Log.d("RECOGNIZER", "onBufferReceived");
        }

        @Override
        public void onEndOfSpeech() {

            Log.d("RECOGNIZER", "onEndOfSpeech");
            recognizer.startListening(intent);

        }

        @Override
        public void onPartialResults(Bundle partialResults) {
            Log.d("RECOGNIZER", "onPartialResults" + partialResults.toString());
        }

        @Override
        public void onEvent(int eventType, Bundle params) {
            Log.d("RECOGNIZER", "onPartialResults" + params.toString());
        }
    }

    @Override
    protected void onDestroy() {
        recognizer.stopListening();
        recognizer.destroy();
        super.onDestroy();
    }

    private void sendMessageToDevice(String msg)
    {
        if (btSocket!=null)
        {
            try
            {
                btSocket.getOutputStream().write(msg.getBytes());
            }
            catch (IOException e)
            {
                msg("Error");
            }
        }
    }

    private class ConnectBT extends AsyncTask<Void, Void, Void>  // UI thread
    {
        private boolean ConnectSuccess = true; //if it's here, it's almost connected

        @Override
        protected void onPreExecute()
        {}

        @Override
        protected Void doInBackground(Void... devices) //while the progress dialog is shown, the connection is done in background
        {
            try
            {
                if (btSocket == null || !isBtConnected)
                {
                    myBluetooth = BluetoothAdapter.getDefaultAdapter();//get the mobile bluetooth device
                    BluetoothDevice dispositivo = myBluetooth.getRemoteDevice(address);//connects to the device's address and checks if it's available
                    btSocket = dispositivo.createInsecureRfcommSocketToServiceRecord(myUUID);//create a RFCOMM (SPP) connection
                    BluetoothAdapter.getDefaultAdapter().cancelDiscovery();
                    btSocket.connect();//start connection
                }
            }
            catch (IOException e)
            {
                ConnectSuccess = false;//if the try failed, you can check the exception here
            }
            return null;
        }
        @Override
        protected void onPostExecute(Void result) //after the doInBackground, it checks if everything went fine
        {
            super.onPostExecute(result);

            if (!ConnectSuccess)
            {
                msg("Connection Failed. Is it a SPP Bluetooth? Try again.");
                finish();
            }
            else
            {
                msg("Connected.");
                isBtConnected = true;
            }
        }
    }

    private void msg(String s)
    {
        Toast.makeText(getApplicationContext(),s,Toast.LENGTH_LONG).show();
    }

    private void Disconnect()
    {
        if (btSocket!=null) //If the btSocket is busy
        {
            try
            {
                btSocket.close(); //close connection
            }
            catch (IOException e)
            { msg("Error");}
        }
        finish(); //return to the first layout
    }

}
