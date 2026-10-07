package com.topsai.meteringsocket;
import org.junit.Test;
import static org.junit.Assert.*;
import java.io.*;
public class StreamsTest {
 @Test public void readsBounded() throws Exception {assertEquals("你好",Streams.read(new ByteArrayInputStream("你好".getBytes("UTF-8"))));try{Streams.read(new ByteArrayInputStream(new byte[32769]));fail();}catch(IOException expected){}}
}
