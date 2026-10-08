package com.topsai.meteringsocket;
import android.test.ActivityInstrumentationTestCase2;
import org.json.*;
public class PanelTest extends ActivityInstrumentationTestCase2<MainActivity>{
 public PanelTest(){super(MainActivity.class);}
 public void testDirectRelayAndInterlock() throws Exception {
  android.content.Context ctx=getInstrumentation().getTargetContext();String original=Vault.load(ctx);
  try{
   JSONArray profiles=new JSONArray();JSONObject p=new JSONObject();p.put("name","模拟插座");p.put("host","http://10.0.2.2:8765");p.put("token","0123456789abcdef0123456789abcdef");profiles.put(p);Vault.save(ctx,profiles.toString());
   MainActivity a=getActivity();String h="http://10.0.2.2:8765",t=p.getString("token");
   a.request(h,t,"/api/config",a.json("rule","manual"));a.request(h,t,"/api/reset",new JSONObject());
   JSONObject s=a.request(h,t,"/api/state",null);getInstrumentation().runOnMainSync(()->a.render(s));assertTrue(a.on.isEnabled());
   getInstrumentation().runOnMainSync(()->a.on.performClick());Thread.sleep(1200);assertTrue(a.request(h,t,"/api/state",null).getBoolean("relay"));
   getInstrumentation().runOnMainSync(()->a.off.performClick());Thread.sleep(1200);assertFalse(a.request(h,t,"/api/state",null).getBoolean("relay"));
   a.request(h,t,"/api/config",a.json("rule","both"));assertFalse(a.request(h,t,"/api/relay",a.json("on",true)).getBoolean("relay"));
   a.request(h,t,"/api/config",a.json("rule","manual"));
   a.request(h,t,"/api/config",a.channelBody("external",a.json("rule","manual")));a.request(h,t,"/api/reset",a.channelBody("external",new JSONObject()));
   JSONObject externalState=a.request(h,t,"/api/state",null);getInstrumentation().runOnMainSync(()->a.render(externalState));assertTrue(a.externalOn.isEnabled());
   getInstrumentation().runOnMainSync(()->a.externalOn.performClick());Thread.sleep(1200);JSONObject externalOn=a.request(h,t,"/api/state",null);assertTrue(externalOn.getJSONObject("external").getBoolean("relay"));assertFalse(externalOn.getBoolean("relay"));
   a.request(h,t,"/api/timer",a.channelBody("external",a.json("seconds",600)));JSONObject timed=a.request(h,t,"/api/state",null);assertTrue(timed.getJSONObject("external").getInt("countdown")>0);assertEquals(0,timed.getInt("countdown"));
   JSONObject config=new JSONObject().put("rule","both").put("max_current",5).put("max_power",1000).put("schedule_on",60).put("schedule_off",120);
   JSONObject independent=a.request(h,t,"/api/config",a.channelBody("external",config));assertEquals("both",independent.getJSONObject("external").getString("rule"));assertEquals("manual",independent.getString("rule"));assertEquals(60,independent.getJSONObject("external").getInt("schedule_on"));
   assertFalse(a.request(h,t,"/api/relay",a.channelBody("external",a.json("on",true))).getJSONObject("external").getBoolean("relay"));
   a.request(h,t,"/api/config",a.channelBody("external",a.json("rule","manual")));a.request(h,t,"/api/relay",a.channelBody("external",a.json("on",true)));
   getInstrumentation().runOnMainSync(()->a.externalOff.performClick());Thread.sleep(1200);assertFalse(a.request(h,t,"/api/state",null).getJSONObject("external").getBoolean("relay"));
   try{a.request(h,"wrong-token","/api/state",null);fail("unauthorized accepted");}catch(java.io.IOException expected){}
   int before=a.request(h,t,"/api/state",null).optInt("ota_count");
   java.io.File firmware=new java.io.File(ctx.getCacheDir(),"test-firmware.bin");try(java.io.FileOutputStream out=new java.io.FileOutputStream(firmware)){out.write(new byte[256]);}
   a.upload(android.net.Uri.fromFile(firmware));Thread.sleep(1200);assertEquals(before+1,a.request(h,t,"/api/state",null).optInt("ota_count"));firmware.delete();
   JSONObject finalState=a.request(h,t,"/api/state",null);getInstrumentation().runOnMainSync(()->a.render(finalState));Thread.sleep(300);
   android.graphics.Bitmap bitmap=getInstrumentation().getUiAutomation().takeScreenshot();try(java.io.FileOutputStream out=new java.io.FileOutputStream(new java.io.File(ctx.getFilesDir(),"panel-test.png"))){bitmap.compress(android.graphics.Bitmap.CompressFormat.PNG,100,out);}bitmap.recycle();
   getInstrumentation().runOnMainSync(()->a.onPause());assertFalse(a.on.isEnabled());
  }finally{Vault.save(ctx,original);}
 }
}
