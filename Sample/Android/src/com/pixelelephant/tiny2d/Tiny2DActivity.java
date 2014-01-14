package com.pixelelephant.tiny2d;

import org.libsdl.app.SDLActivity;

/**
    Tiny2D Activity
*/
public class Tiny2DActivity extends SDLActivity {
	static {
        System.loadLibrary("Tiny2D");
        System.loadLibrary("Tiny2DApp");
	}
}

