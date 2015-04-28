/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ambimmort.sdncenter.util;

import java.net.URI;
import org.apache.http.annotation.NotThreadSafe;
import org.apache.http.client.methods.HttpEntityEnclosingRequestBase;

/**
 *
 * @author Administrator
 */
@NotThreadSafe
public class RestDeleteMethod extends HttpEntityEnclosingRequestBase{
    
    public final static String METHOD_NAME = "DELETE";
    
    public RestDeleteMethod(){
        super();
    }
    
    public RestDeleteMethod(final URI uri){
        super();
        setURI(uri);
    }
    
    public RestDeleteMethod(final String uri){
        super();
        setURI(URI.create(uri));
    }
    
    @Override
    public String getMethod() {
        return METHOD_NAME;
    }
    
}
