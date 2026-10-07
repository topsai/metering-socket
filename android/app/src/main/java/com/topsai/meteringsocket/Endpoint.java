package com.topsai.meteringsocket;
import java.net.URI;
public final class Endpoint {
 public static String normalize(String value){
  String s=value.trim(); if(!s.contains("://"))s="http://"+s;
  URI u=URI.create(s); if(!("http".equals(u.getScheme())||"https".equals(u.getScheme()))||u.getHost()==null||u.getUserInfo()!=null||u.getQuery()!=null||u.getFragment()!=null||!(u.getPath().isEmpty()||u.getPath().equals("/")))throw new IllegalArgumentException("请输入设备 IP 或域名，不带路径");
  return s.endsWith("/")?s.substring(0,s.length()-1):s;
 }
}
