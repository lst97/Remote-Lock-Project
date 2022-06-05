package com.nenoff.connecttoarduinoble;

import java.nio.ByteBuffer;

import dev.samstevens.totp.code.CodeGenerator;
import dev.samstevens.totp.code.DefaultCodeGenerator;
import dev.samstevens.totp.code.HashingAlgorithm;
import dev.samstevens.totp.exceptions.CodeGenerationException;
import dev.samstevens.totp.time.SystemTimeProvider;
import dev.samstevens.totp.time.TimeProvider;

public class RemoteControl {
    private final BLEController bleController;

    public RemoteControl(BLEController bleController) {
        this.bleController = bleController;
    }

    private String get_totp(){
        CodeGenerator codeGenerator = new DefaultCodeGenerator(HashingAlgorithm.SHA1);
        TimeProvider timeProvider = new SystemTimeProvider();
        try {
            return codeGenerator.generate("JBQU442PNVIHET2NMV5GIYJRGIYUYU2UHE3Q====", Math.floorDiv(timeProvider.getTime(), 30));
        } catch (CodeGenerationException e) {
            e.printStackTrace();
        }
        return "";
    }
    public void locking(byte cmd){
        int totp_int = Integer.parseInt(get_totp());
        byte[] bytes = ByteBuffer.allocate(4).putInt(totp_int).array();

        byte[] b = new byte[5];
        b[0] = 1;
        b[1] = cmd;
        b[2] = bytes[1];
        b[3] = bytes[2];
        b[4] = bytes[3];

        this.bleController.sendData(b);
    }
}
