package com.example.securemodule

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import com.example.securemodule.databinding.ActivityMainBinding
import java.util.*
import kotlin.system.exitProcess

class MainActivity : AppCompatActivity(), IAntiDebugCallback {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        binding.start.setOnClickListener {
            AntiDebug.startCheckDebug(this)
        }
        binding.stop.setOnClickListener {
            AntiDebug.stopCheck()
        }

    }

    /**
     * A native method that is implemented by the 'securemodule' native library,
     * which is packaged with this application.
     */
//    external fun stringFromJNI(): String

//    companion object {
//        // Used to load the 'securemodule' library on application startup.
//        init {
//            System.loadLibrary("securemodule")
//        }
//    }


    override fun beInjectedDebug() {

        runOnUiThread {
            val decode = android.util.Base64.decode(
                "6K+35YGc5q2i6Z2e5rOV5YWl5L6177yM5aaC5p6c5oKo5pyJ5YW06Laj77yM5qyi6L+O5Yqg5YWl6I2U5pSvIGZhbm1pbmd5aUBsaXpoaS5mbQ==",
                android.util.Base64.DEFAULT
            ).decodeToString()



            AlertDialog
                .Builder(this)
                .setMessage(decode).create().show()

            Handler(Looper.getMainLooper()).postDelayed(Runnable { exitProcess(0) },3000)


        }

        println("MainActivity.beInjectedDebug")
    }
}