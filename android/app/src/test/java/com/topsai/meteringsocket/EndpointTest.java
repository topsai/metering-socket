package com.topsai.meteringsocket;
import org.junit.Test;
import static org.junit.Assert.*;
public class EndpointTest {
 @Test public void addresses(){assertEquals("http://192.168.4.1",Endpoint.normalize("192.168.4.1/"));assertEquals("https://socket.example",Endpoint.normalize("https://socket.example"));}
 @Test public void rejects(){for(String s:new String[]{"file:///etc","http://user:pass@host","http://host/api","http://host?x=1","javascript:alert(1)"}){try{Endpoint.normalize(s);fail(s);}catch(IllegalArgumentException expected){}}}
}
