package com.topsai.meteringsocket;
import android.test.ActivityInstrumentationTestCase2;
import org.json.*;
public class DualPanelTest extends ActivityInstrumentationTestCase2<MainActivity>{
 public DualPanelTest(){super(MainActivity.class);}
 public void testIndependentStateAndChannelPayload() throws Exception {
  MainActivity a=getActivity();
  JSONObject external=new JSONObject().put("relay",true).put("rule","manual").put("countdown",33);
  JSONObject s=new JSONObject().put("meter_valid",true).put("relay",false).put("rule","manual").put("latch_known",true).put("latch_estimated",true).put("external",external).put("cf1_count",12345678901L).put("cf1_frequency",7.25).put("led_on",true);
  getInstrumentation().runOnMainSync(()->a.render(s));
  assertTrue(a.relay.getText().toString().contains("估计开启"));
  assertTrue(a.externalRelay.getText().toString().contains("已开启"));
  assertTrue(a.externalRelay.getText().toString().contains("33"));
  assertTrue(a.inputs.getText().toString().contains("12345678901"));
  assertTrue(a.inputs.getText().toString().contains("7.25"));
  JSONObject body=a.channelBody("external",a.json("on",false));
  assertEquals("external",body.getString("channel"));assertFalse(body.getBoolean("on"));
  assertEquals("onboard",a.channelBody("onboard",a.json("seconds",600)).getString("channel"));
  external.put("fault",true);s.put("latch_busy",true);
  getInstrumentation().runOnMainSync(()->a.render(s));
  assertFalse(a.externalOn.isEnabled());assertTrue(a.externalReset.isEnabled());assertFalse(a.on.isEnabled());
  getInstrumentation().runOnMainSync(()->a.disable());
  assertFalse(a.externalOff.isEnabled());assertFalse(a.externalTimer.isEnabled());
 }
 public void testUnknownLatchAndMissingExternalStayUnavailable() throws Exception {
  MainActivity a=getActivity();JSONObject s=new JSONObject().put("meter_valid",true).put("rule","manual");
  getInstrumentation().runOnMainSync(()->a.render(s));
  assertTrue(a.relay.getText().toString().contains("未知"));assertFalse(a.externalOn.isEnabled());assertFalse(a.externalOff.isEnabled());
 }
 public void testLatchPulseBounds() throws Exception {
  MainActivity a=getActivity();assertEquals(50,a.parseLatchPulse("50"));assertEquals(200,a.parseLatchPulse("200"));
  for(String value:new String[]{"49","201","abc"}){try{a.parseLatchPulse(value);fail("invalid latch pulse accepted");}catch(IllegalArgumentException expected){}}
 }

}
