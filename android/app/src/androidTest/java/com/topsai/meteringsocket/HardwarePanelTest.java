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
   assertTrue(s.has("cf1_count"));assertTrue(s.has("cf1_frequency"));assertNotNull(s.optJSONObject("external"));
   for(String channel:new String[]{"onboard","external"}){a.request(h,t,"/api/config",a.channelBody(channel,a.json("rule","manual")));a.request(h,t,"/api/reset",a.channelBody(channel,new JSONObject()));a.request(h,t,"/api/relay",a.channelBody(channel,a.json("on",false)));}
   Thread.sleep(1500);s=a.request(h,t,"/api/state",null);final JSONObject before=s;getInstrumentation().runOnMainSync(()->a.render(before));
   assertTrue(a.on.isEnabled());assertTrue(a.externalOn.isEnabled());
   getInstrumentation().runOnMainSync(()->a.on.performClick());Thread.sleep(1500);JSONObject boardOn=a.request(h,t,"/api/state",null);assertTrue(boardOn.getBoolean("relay"));assertTrue(boardOn.getBoolean("latch_known"));assertTrue(boardOn.getBoolean("latch_estimated"));assertFalse(boardOn.getBoolean("latch_busy"));assertFalse(boardOn.getJSONObject("external").getBoolean("relay"));
   getInstrumentation().runOnMainSync(()->a.off.performClick());Thread.sleep(1500);JSONObject boardOff=a.request(h,t,"/api/state",null);assertFalse(boardOff.getBoolean("relay"));assertFalse(boardOff.getBoolean("latch_estimated"));assertFalse(boardOff.getJSONObject("external").getBoolean("relay"));
   getInstrumentation().runOnMainSync(()->a.externalOn.performClick());Thread.sleep(1500);JSONObject extOn=a.request(h,t,"/api/state",null);assertTrue(extOn.getJSONObject("external").getBoolean("relay"));assertFalse(extOn.getBoolean("relay"));assertFalse(extOn.getBoolean("latch_estimated"));
   getInstrumentation().runOnMainSync(()->a.externalOff.performClick());Thread.sleep(1500);JSONObject extOff=a.request(h,t,"/api/state",null);assertFalse(extOff.getJSONObject("external").getBoolean("relay"));assertFalse(extOff.getBoolean("relay"));
   final JSONObject after=a.request(h,t,"/api/state",null);getInstrumentation().runOnMainSync(()->a.render(after));Thread.sleep(2200);assertTrue("Live polling must remain connected",a.connection.getText().toString().startsWith("已连接"));
   android.graphics.Bitmap bitmap=getInstrumentation().getUiAutomation().takeScreenshot();try(java.io.FileOutputStream out=new java.io.FileOutputStream(new java.io.File(ctx.getFilesDir(),"hardware-test.png"))){bitmap.compress(android.graphics.Bitmap.CompressFormat.PNG,100,out);}bitmap.recycle();
  } finally{Vault.save(ctx,original);fixture.delete();}
 }
}
