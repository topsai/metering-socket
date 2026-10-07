package com.topsai.meteringsocket;
import java.io.*;
import java.nio.charset.StandardCharsets;
final class Streams {
 static String read(InputStream input) throws IOException {try(InputStream in=input;ByteArrayOutputStream out=new ByteArrayOutputStream()){byte[] b=new byte[2048];int n;while((n=in.read(b))!=-1){if(out.size()+n>32768)throw new IOException("设备响应过大");out.write(b,0,n);}return new String(out.toByteArray(),StandardCharsets.UTF_8);}}
}
