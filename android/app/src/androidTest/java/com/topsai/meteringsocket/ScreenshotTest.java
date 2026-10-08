package com.topsai.meteringsocket;
import android.test.ActivityInstrumentationTestCase2;
import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.graphics.Bitmap;
import android.view.KeyEvent;
import android.widget.ScrollView;
import java.io.File;
import java.io.FileOutputStream;
import org.json.*;

/** Captures the actual 1.1.1 views with public, synthetic demonstration data. */
public class ScreenshotTest extends ActivityInstrumentationTestCase2<MainActivity> {
 public ScreenshotTest(){super(MainActivity.class);}
 MainActivity a;
 void shot(String name) throws Exception {
  getInstrumentation().waitForIdleSync();Thread.sleep(700);
  Bitmap bitmap=getInstrumentation().getUiAutomation().takeScreenshot();assertNotNull(bitmap);
  File dir=new File(a.getExternalFilesDir(null),"screenshots");dir.mkdirs();
  try(FileOutputStream out=new FileOutputStream(new File(dir,name+".png"))){bitmap.compress(Bitmap.CompressFormat.PNG,100,out);}bitmap.recycle();
 }
 void scroll(int card) {getInstrumentation().runOnMainSync(()->((ScrollView)a.root.getParent()).scrollTo(0,a.root.getChildAt(card).getTop()));}
 void dialog(String name,Runnable action) throws Exception {getInstrumentation().runOnMainSync(action);shot(name);getInstrumentation().sendKeyDownUpSync(KeyEvent.KEYCODE_BACK);}
 public void testCapturePages() throws Exception {
  a=getActivity();
  JSONObject ext=new JSONObject().put("relay",false).put("rule","manual").put("reason","manual").put("max_current",10).put("max_power",2200).put("schedule_on",-1).put("schedule_off",-1);
  JSONObject s=new JSONObject(ext.toString()).put("external",ext).put("ip","192.168.1.100").put("meter_valid",true).put("voltage",230).put("current",0.25).put("power",57.5).put("frequency",50).put("energy",0.125).put("input1",true).put("input2",false).put("cf1_count",42).put("cf1_frequency",2.5).put("led_on",true).put("latch_known",true).put("latch_estimated",false).put("latch_on_ina",true).put("latch_pulse_ms",100);
  getInstrumentation().runOnMainSync(()->{a.active=false;a.host="";a.generation++;a.profiles=new JSONArray();a.updateDevices();a.latest=s;a.render(s);});
  scroll(0);shot("01-dashboard");scroll(4);shot("02-onboard");scroll(5);shot("03-external");scroll(6);shot("04-settings");
  dialog("05-device",()->a.deviceDialog());
  dialog("06-onboard-rules",()->a.rulesDialog("onboard"));
  dialog("07-external-rules",()->a.rulesDialog("external"));
  dialog("08-onboard-timer",()->a.timerDialog("onboard"));
  dialog("09-external-timer",()->a.timerDialog("external"));
  dialog("10-calibration",()->a.hardwareDialog());
  dialog("11-network",()->a.networkDialog());
  dialog("12-home-assistant",()->a.haDialog());
  dialog("13-firmware-upgrade",()->a.onActivityResult(20,Activity.RESULT_OK,new Intent().setData(Uri.parse("content://demo/firmware.bin"))));
 }
}
