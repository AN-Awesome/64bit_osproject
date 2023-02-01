#include "RTC.h"

// Read current time stored by RTC controller from CMOS memory
void kReadRTCTime(BYTE* pbHour, BYTE* pbMinute, BYTE* pbSecond) {
    BYTE bData;
    
    // Specify time save register
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_HOUR);
    // Read Time
    bData = kInPortByte(RTC_CMOSDATA);
    *pbHour = RTC_BCDTOBINARY(bData);

    // Specify minutes save register
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MINUTE);
    // Read minutes
    bData = kInPortByte(RTC_CMOSDATA);
    *pbMinute = RTC_BCDTOBINARY(bData);

    // Specify seconds save register
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_SECOND);
    // Read seconds
    bData = kInPortByte(RTC_CMOSDATA);
    *pbSecond = RTC_BCDTOBINARY(bData);
}

//read current data
void kReadRTCDate(WORD* pwYear, BYTE* pbMonth, BYTE* pbDayOfMonth, BYTE* pbDayOfWeek) {
    BYTE bData;

    // Specify register to store year
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_YEAR);
    // Read Year
    bData = kInPortByte(RTC_CMOSDATA );
    *pwYear = RTC_BCDTOBINARY(bData) + 2000;

    // Specify register to store month
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MONTH);
    // Read Month
    bData = kInPortByte(RTC_CMOSDATA);
    *pbMonth = RTC_BCDTOBINARY(bData);

    // Specify register to store day
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFMONTH);
    // Read Day
    bData = kInPortByte(RTC_CMOSDATA);
    *pbDayOfMonth = RTC_BCDTOBINARY(bData);

    // Specify register to Store day of the week
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFWEEK);
    // Read Day of the week
    bData = kInPortByte(RTC_CMOSDATA);
    *pbDayOfWeek = RTC_BCDTOBINARY(bData);
}

// Conver to String
char* kConvertDayOfWeekToString(BYTE bDayOfWeek) {
    static char* vpcDayOfWeekString[8] = {"Error", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

    //Return Error if not in Range
    if(bDayOfWeek >= 8) return vpcDayOfWeekString[0];

    //Return Day of the Week
    return vpcDayOfWeekString[bDayOfWeek];
}