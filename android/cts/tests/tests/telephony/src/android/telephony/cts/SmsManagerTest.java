/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.telephony.cts;


import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.SystemClock;
import android.provider.Telephony;
import android.telephony.SmsMessage;
import android.telephony.TelephonyManager;
import android.test.AndroidTestCase;
import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Tests for {@link android.telephony.SmsManager}.
 *
 * Structured so tests can be reused to test {@link android.telephony.gsm.SmsManager}
 */
public class SmsManagerTest extends AndroidTestCase {

    private static final String TAG = "SmsManagerTest";
    private static final String LONG_TEXT =
        "This is a very long text. This text should be broken into three " +
        "separate messages.This is a very long text. This text should be broken into " +
        "three separate messages.This is a very long text. This text should be broken " +
        "into three separate messages.This is a very long text. This text should be " +
        "broken into three separate messages.";;
    private static final String LONG_TEXT_WITH_32BIT_CHARS =
        "Long dkkshsh jdjsusj kbsksbdf jfkhcu hhdiwoqiwyrygrvn?*?*!\";:'/,."
        + "__?9#9292736&4;\"$+$+((]\\[\\℅©℅™^®°¥°¥=¢£}}£∆~¶~÷|√×."
        + " 😯😆😉😇😂😀👕🎓😀👙🐕🐀🐶🐰🐩⛪⛲ ";

    private static final String SMS_SEND_ACTION = "CTS_SMS_SEND_ACTION";
    private static final String SMS_DELIVERY_ACTION = "CTS_SMS_DELIVERY_ACTION";
    private static final String DATA_SMS_RECEIVED_ACTION = "android.intent.action.DATA_SMS_RECEIVED";
    public static final String SMS_DELIVER_DEFAULT_APP_ACTION = "CTS_SMS_DELIVERY_ACTION_DEFAULT_APP";

    // List of network operators that don't support SMS delivery report
    private static final List<String> NO_DELIVERY_REPORTS =
            Arrays.asList(
                    "310410",   // AT&T Mobility
                    "44010",    // NTT DOCOMO
                    "45005",    // SKT Mobility
                    "45002",    // SKT Mobility
                    "45008",    // KT Mobility
                    "45006",    // LGT
                    "311660",   // MetroPCS
                    "310120",   // Sprint
                    "44050",    // KDDI
                    "44051",    // KDDI
                    "44053",    // KDDI
                    "44054",    // KDDI
                    "44070",    // KDDI
                    "44071",    // KDDI
                    "44072",    // KDDI
                    "44073",    // KDDI
                    "44074",    // KDDI
                    "44075",    // KDDI
                    "44076",    // KDDI
                    "50502",    // OPS
                    "51502",    // Globe Telecoms
                    "51503",    // Smart Communications
                    "51505",    // Sun Cellular
                    "53001",    // Vodafone New Zealand
                    "53024",    // NZC
                    "311870",   // Boost Mobile
                    "311220",   // USCC
                    "311225",   // USCC LTE
                    "311580",   // USCC LTE
                    "302720",   // Rogers
                    "30272",    // Rogers
                    "302370",   // Fido
                    "30237",    // Fido
                    "311490",   // Virgin Mobile
                    "312530",   // Sprint Prepaid
                    "310000",   // Tracfone
                    "46003",    // China Telecom
                    "311230",   // C SPire Wireless + Celluar South
                    "310600",    // Cellcom
                    "31000",     // Republic Wireless US
                    "310260",    // Republic Wireless US
                    "310026",     // T-Mobile US
                    "330120", // OpenMobile communication
                    // Verizon
                    "310004",
                    "310012",
                    "311280",
                    "311281",
                    "311282",
                    "311283",
                    "311284",
                    "311285",
                    "311286",
                    "311287",
                    "311288",
                    "311289",
                    "311480",
                    "311481",
                    "311482",
                    "311483",
                    "311484",
                    "311485",
                    "311486",
                    "311487",
                    "311488",
                    "311489"
            );

    // List of network operators that doesn't support Data(binary) SMS message
    private static final List<String> UNSUPPORT_DATA_SMS_MESSAGES =
            Arrays.asList(
                    "44010",    // NTT DOCOMO
                    "44020",    // SBM
                    "44051",    // KDDI
                    "302720",   // Rogers
                    "30272",    // Rogers
                    "302370",   // Fido
                    "30237",    // Fido
                    "45008",    // KT
                    "45005",    // SKT Mobility
                    "45002",     // SKT Mobility
                    "45006",    // LGT
                    "310260",   // Republic Wireless US
                    // Verizon
                    "310004",
                    "310012",
                    "311280",
                    "311281",
                    "311282",
                    "311283",
                    "311284",
                    "311285",
                    "311286",
                    "311287",
                    "311288",
                    "311289",
                    "311480",
                    "311481",
                    "311482",
                    "311483",
                    "311484",
                    "311485",
                    "311486",
                    "311487",
                    "311488",
                    "311489"
            );

    // List of network operators that doesn't support Maltipart SMS message
    private static final List<String> UNSUPPORT_MULTIPART_SMS_MESSAGES =
            Arrays.asList(
                    "44010",    // NTT DOCOMO
                    "44020",    // SBM
                    "44051",    // KDDI
                    "302720",   // Rogers
                    "30272",    // Rogers
                    "302370",   // Fido
                    "30237",    // Fido
                    "45006",    // LGT
                    "45008"     // KT
            );

    private TelephonyManager mTelephonyManager;
    private PackageManager mPackageManager;
    private String mDestAddr;
    private String mText;
    private SmsBroadcastReceiver mSendReceiver;
    private SmsBroadcastReceiver mDeliveryReceiver;
    private SmsBroadcastReceiver mDataSmsReceiver;
    private SmsBroadcastReceiver mSmsDeliverReceiver;
    private SmsBroadcastReceiver mSmsReceivedReceiver;
    private PendingIntent mSentIntent;
    private PendingIntent mDeliveredIntent;
    private Intent mSendIntent;
    private Intent mDeliveryIntent;
    private boolean mDeliveryReportSupported;
    private static boolean mReceivedDataSms;
    private static String mReceivedText;

    private static final int TIME_OUT = 1000 * 60 * 5;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mTelephonyManager =
            (TelephonyManager) getContext().getSystemService(Context.TELEPHONY_SERVICE);
        mPackageManager = mContext.getPackageManager();
        mDestAddr = mTelephonyManager.getLine1Number();
        mText = "This is a test message";

        if (!mPackageManager.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            mDeliveryReportSupported = false;
        } else {
            // exclude the networks that don't support SMS delivery report
            String mccmnc = mTelephonyManager.getSimOperator();
            mDeliveryReportSupported = !(NO_DELIVERY_REPORTS.contains(mccmnc));
        }
    }

    public void testDivideMessage() {
        ArrayList<String> dividedMessages = divideMessage(LONG_TEXT);
        assertNotNull(dividedMessages);
        if (TelephonyUtils.isSkt(mTelephonyManager)) {
            assertTrue(isComplete(dividedMessages, 5, LONG_TEXT)
                    || isComplete(dividedMessages, 3, LONG_TEXT));
        } else if (TelephonyUtils.isKt(mTelephonyManager)) {
            assertTrue(isComplete(dividedMessages, 4, LONG_TEXT)
                    || isComplete(dividedMessages, 3, LONG_TEXT));
        } else {
            assertTrue(isComplete(dividedMessages, 3, LONG_TEXT));
        }
    }

    public void testDivideUnicodeMessage() {
        ArrayList<String> dividedMessages = divideMessage(LONG_TEXT_WITH_32BIT_CHARS);
        assertNotNull(dividedMessages);
        assertTrue(isComplete(dividedMessages, 3, LONG_TEXT_WITH_32BIT_CHARS));
        for (String messagePiece : dividedMessages) {
            assertFalse(Character.isHighSurrogate(
                    messagePiece.charAt(messagePiece.length() - 1)));
        }
    }

    private boolean isComplete(List<String> dividedMessages, int numParts, String longText) {
        if (dividedMessages.size() != numParts) {
            return false;
        }

        String actualMessage = "";
        for (int i = 0; i < numParts; i++) {
            actualMessage += dividedMessages.get(i);
        }
        return longText.equals(actualMessage);
    }

    public void testSendAndReceiveMessages() throws InterruptedException {
        if (!mPackageManager.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            return;
        }

        String mccmnc = mTelephonyManager.getSimOperator();

        mSendIntent = new Intent(SMS_SEND_ACTION);
        mDeliveryIntent = new Intent(SMS_DELIVERY_ACTION);

        IntentFilter sendIntentFilter = new IntentFilter(SMS_SEND_ACTION);
        IntentFilter deliveryIntentFilter = new IntentFilter(SMS_DELIVERY_ACTION);
        IntentFilter dataSmsReceivedIntentFilter = new IntentFilter(DATA_SMS_RECEIVED_ACTION);
        IntentFilter smsDeliverIntentFilter = new IntentFilter(SMS_DELIVER_DEFAULT_APP_ACTION);
        IntentFilter smsReceivedIntentFilter =
                new IntentFilter(Telephony.Sms.Intents.SMS_RECEIVED_ACTION);
        dataSmsReceivedIntentFilter.addDataScheme("sms");
        dataSmsReceivedIntentFilter.addDataAuthority("localhost", "19989");

        mSendReceiver = new SmsBroadcastReceiver(SMS_SEND_ACTION);
        mDeliveryReceiver = new SmsBroadcastReceiver(SMS_DELIVERY_ACTION);
        mDataSmsReceiver = new SmsBroadcastReceiver(DATA_SMS_RECEIVED_ACTION);
        mSmsDeliverReceiver = new SmsBroadcastReceiver(SMS_DELIVER_DEFAULT_APP_ACTION);
        mSmsReceivedReceiver = new SmsBroadcastReceiver(Telephony.Sms.Intents.SMS_RECEIVED_ACTION);

        getContext().registerReceiver(mSendReceiver, sendIntentFilter);
        getContext().registerReceiver(mDeliveryReceiver, deliveryIntentFilter);
        getContext().registerReceiver(mDataSmsReceiver, dataSmsReceivedIntentFilter);
        getContext().registerReceiver(mSmsDeliverReceiver, smsDeliverIntentFilter);
        getContext().registerReceiver(mSmsReceivedReceiver, smsReceivedIntentFilter);

        // send single text sms
        init();
        sendTextMessage(mDestAddr, mDestAddr, mSentIntent, mDeliveredIntent);
        assertTrue(mSendReceiver.waitForCalls(1, TIME_OUT));
        if (mDeliveryReportSupported) {
            assertTrue(mDeliveryReceiver.waitForCalls(1, TIME_OUT));
        }
        // non-default app should receive only SMS_RECEIVED_ACTION
        assertTrue(mSmsReceivedReceiver.waitForCalls(1, TIME_OUT));
        assertTrue(mSmsDeliverReceiver.waitForCalls(0, 0));

        // due to permission restrictions, currently there is no way to make this test app the
        // default SMS app

        if (mTelephonyManager.getPhoneType() == TelephonyManager.PHONE_TYPE_CDMA) {
            // TODO: temp workaround, OCTET encoding for EMS not properly supported
            return;
        }

        // send data sms
        if (!UNSUPPORT_DATA_SMS_MESSAGES.contains(mccmnc)) {
            byte[] data = mText.getBytes();
            short port = 19989;

            init();
            sendDataMessage(mDestAddr, port, data, mSentIntent, mDeliveredIntent);
            assertTrue(mSendReceiver.waitForCalls(1, TIME_OUT));
            if (mDeliveryReportSupported) {
                assertTrue(mDeliveryReceiver.waitForCalls(1, TIME_OUT));
            }
            mDataSmsReceiver.waitForCalls(1, TIME_OUT);
            assertTrue(mReceivedDataSms);
            assertEquals(mReceivedText, mText);
        } else {
            // This GSM network doesn't support Data(binary) SMS message.
            // Skip the test.
        }

        // send multi parts text sms
        if (!UNSUPPORT_MULTIPART_SMS_MESSAGES.contains(mccmnc)) {
            init();
            ArrayList<String> parts = divideMessage(LONG_TEXT);
            int numParts = parts.size();
            ArrayList<PendingIntent> sentIntents = new ArrayList<PendingIntent>();
            ArrayList<PendingIntent> deliveryIntents = new ArrayList<PendingIntent>();
            for (int i = 0; i < numParts; i++) {
                sentIntents.add(PendingIntent.getBroadcast(getContext(), 0, mSendIntent, 0));
                deliveryIntents.add(PendingIntent.getBroadcast(getContext(), 0, mDeliveryIntent, 0));
            }
            sendMultiPartTextMessage(mDestAddr, parts, sentIntents, deliveryIntents);
            assertTrue(mSendReceiver.waitForCalls(numParts, TIME_OUT));
            if (mDeliveryReportSupported) {
              assertTrue(mDeliveryReceiver.waitForCalls(numParts, TIME_OUT));
            }
            // non-default app should receive only SMS_RECEIVED_ACTION
            assertTrue(mSmsReceivedReceiver.waitForCalls(1, TIME_OUT));
            assertTrue(mSmsDeliverReceiver.waitForCalls(0, 0));
        } else {
            // This GSM network doesn't support Multipart SMS message.
            // Skip the test.
        }
    }

    private void init() {
        mSendReceiver.reset();
        mDeliveryReceiver.reset();
        mDataSmsReceiver.reset();
        mSmsDeliverReceiver.reset();
        mSmsReceivedReceiver.reset();
        mReceivedDataSms = false;
        mSentIntent = PendingIntent.getBroadcast(getContext(), 0, mSendIntent,
                PendingIntent.FLAG_ONE_SHOT);
        mDeliveredIntent = PendingIntent.getBroadcast(getContext(), 0, mDeliveryIntent,
                PendingIntent.FLAG_ONE_SHOT);
    }

    public void testGetDefault() {
        assertNotNull(getSmsManager());
    }

    protected ArrayList<String> divideMessage(String text) {
        return getSmsManager().divideMessage(text);
    }

    private android.telephony.SmsManager getSmsManager() {
        return android.telephony.SmsManager.getDefault();
    }

    protected void sendMultiPartTextMessage(String destAddr, ArrayList<String> parts,
            ArrayList<PendingIntent> sentIntents, ArrayList<PendingIntent> deliveryIntents) {
        getSmsManager().sendMultipartTextMessage(destAddr, null, parts, sentIntents, deliveryIntents);
    }

    protected void sendDataMessage(String destAddr,short port, byte[] data, PendingIntent sentIntent, PendingIntent deliveredIntent) {
        getSmsManager().sendDataMessage(destAddr, null, port, data, sentIntent, deliveredIntent);
    }

    protected void sendTextMessage(String destAddr, String text, PendingIntent sentIntent, PendingIntent deliveredIntent) {
        getSmsManager().sendTextMessage(destAddr, null, text, sentIntent, deliveredIntent);
    }

    private static class SmsBroadcastReceiver extends BroadcastReceiver {
        private int mCalls;
        private int mExpectedCalls;
        private String mAction;
        private Object mLock;

        SmsBroadcastReceiver(String action) {
            mAction = action;
            reset();
            mLock = new Object();
        }

        void reset() {
            mExpectedCalls = Integer.MAX_VALUE;
            mCalls = 0;
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            if(mAction.equals(DATA_SMS_RECEIVED_ACTION)){
                StringBuilder sb = new StringBuilder();
                Bundle bundle = intent.getExtras();
                if (bundle != null) {
                    Object[] obj = (Object[]) bundle.get("pdus");
                    String format = bundle.getString("format");
                    SmsMessage[] message = new SmsMessage[obj.length];
                    for (int i = 0; i < obj.length; i++) {
                        message[i] = SmsMessage.createFromPdu((byte[]) obj[i], format);
                    }

                    for (SmsMessage currentMessage : message) {
                        byte[] binaryContent = currentMessage.getUserData();
                        String readableContent = new String(binaryContent);
                        sb.append(readableContent);
                    }
                }
                mReceivedDataSms = true;
                mReceivedText=sb.toString();
            }
            Log.i(TAG, "onReceive " + intent.getAction());
            if (intent.getAction().equals(mAction)) {
                synchronized (mLock) {
                    mCalls += 1;
                    mLock.notify();
                }
            }
        }

        public boolean waitForCalls(int expectedCalls, long timeout) throws InterruptedException {
            synchronized(mLock) {
                mExpectedCalls = expectedCalls;
                long startTime = SystemClock.elapsedRealtime();

                while (mCalls < mExpectedCalls) {
                    long waitTime = timeout - (SystemClock.elapsedRealtime() - startTime);
                    if (waitTime > 0) {
                        mLock.wait(waitTime);
                    } else {
                        return false;  // timed out
                    }
                }
                return true;  // success
            }
        }
    }
}
