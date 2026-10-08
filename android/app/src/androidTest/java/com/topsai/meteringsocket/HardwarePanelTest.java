package com.topsai.meteringsocket;
import android.test.ActivityInstrumentationTestCase2;
import org.json.*;
public class HardwarePanelTest extends ActivityInstrumentationTestCase2<MainActivity>{
 public HardwarePanelTest(){super(MainActivity.class);}
 public void testLiveBoardControls() throws Exception {
  android.content.Context ctx=getInstrumentation().getTargetContext();java.io.File fixture=new java.io.File(ctx.getFilesDir(),"bench-device.json");
  if(!fixture.exists())return; // Opt in only: default tests never control a physical board.
  JSONObject device=new JSONObject(Streams.read(new java.io.FileInputStream(fixture)));String original=Vault.load(ctx);
  try{
   JSONObject profile=new JSONObject();profile.put("name","ESP32-C3 实板 · 测试注入");profile.put("host",device.getString("host"));profile.put("token",device.getString("token"));Vault.save(ctx,new JSONArray().put(profile).toString());
   MainActivity a=getActivity();String h=device.getString("host"),t=device.getString("token");JSONObject s=a.request(h,t,"/api/state",null);
   assertTrue("Bench firmware required",s.optBoolean("bench_mode"));assertTrue(s.optBoolean("meter_valid"));
   a.request(h,t,"/api/config",a.json("rule","manual"));a.request(h,t,"/api/reset",new JSONObject());s=a.request(h,t,"/api/state",null);final JSONObject before=s;getInstrumentation().runOnMainSync(()->a.render(before));
   getInstrumentation().runOnMainSync(()->a.on.performClick());Thread.sleep(1500);assertTrue(a.request(h,t,"/api/state",null).getBoolean("relay"));
   getInstrumentation().runOnMainSync(()->a.off.performClick());Thread.sleep(1500);assertFalse(a.request(h,t,"/api/state",null).getBoolean("relay"));
   final JSONObject after=a.request(h,t,"/api/state",null);getInstrumentation().runOnMainSync(()->a.render(after));Thread.sleep(2200);
   android.graphics.Bitmap bitmap=getInstrumentation().getUiAutomation().takeScreenshot();try(java.io.FileOutputStream out=new java.io.FileOutputStream(new java.io.File(ctx.getFilesDir(),"hardware-test.png"))){bitmap.compress(android.graphics.Bitmap.CompressFormat.PNG,100,out);}bitmap.recycle();
  } finally{Vault.save(ctx,original);fixture.delete();}
 }
}
