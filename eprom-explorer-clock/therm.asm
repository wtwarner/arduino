; Boy Scout Explorer 1984 project
; sponsored by Hewlett-Packard, Avondale, PA.

; Copied from paper listing dated Thu, Jun 14, 1984
; code notes:
; - original used ' in names; this copy uses _; for example RCV'DATA vs. RCV_DATA.
; - original used STAD, LDAD mnemonics; this uses STD, LDD
;
; The following features are mentioned in code but do not appear
; to be implemented yet:
; - serial communication
; - alarms (nothing on schematic using these pins)

                    processor           6803
;;; PAGE 1 HARDWARE DEFINITIONS
    
;;; INTERNAL HARDWARE DEFINTIONS
ACC0                EQU     $01
ACC1                EQU     $02
ACC2                EQU     $04
ACC3                EQU     $08
ACC4                EQU     $10
ACC5                EQU     $20
ACC6                EQU     $40
ACC7                EQU     $80

TIMER_CONTROL       EQU     $08             ; timer interrupt control bits, flags, etc.
TIMER_COUNTER       EQU     $09             ; And $0a - timer/counter register
TIMER_OCR_REG       EQU     $0B             ; And $0c - timer output compare register
TIMER_ICR_REG       EQU     $0D             ; And $oe - timer input capture register
TIMER_EOCI          EQU     ACC3
TIMER_OCF           EQU     ACC6
TIMER_OLVL          EQU     ACC0            ; output compare level bit

SER_MODE_REG        EQU     $10             ; mode control register for serial i/o
SER_STAT_REG        EQU     $11             ; status register for serial i/o
RCV_DATA            EQU     $12             ; data register for serial i/o
XMIT_DATA           EQU     $13             ; data register for serial transmitter
RCV_ENABLE          EQU     ACC3            ; enable bit for receiver
XMIT_ENABLE         EQU     ACC1            ; enable bit for transmitter
RCV_INT_EN          EQU     ACC4            ; enable bit for receiver interrupt
XMIT_INT_EN         EQU     ACC2            ; enable bit for transmitter interrupt
RCV_FLAG            EQU     ACC7            ; flag to indicate a byte has been received
XMIT_FLAG           EQU     ACC5            ; flag to indicate that the transmitter is ready to
                                            ;  send another byte
OVERUN_FRAME        EQU     ACC6            ; flag to indicate that either an overrun error
                                            ;  or a framing error has occured

;;;  EXTERNAL HARDWARE DEFINTIONS
PORT1               EQU     $02             ; PARALLEL PORT 1 (ALL INPUTS)
DST_JUMPER          EQU     ACC5            ; jumper to specify daylight saving time
JUMPER_24           EQU     ACC6            ; jumper to specify 24 hour time display
MUX                 EQU     $1000           ; analog multiplexer select bits
DIGIT_DRIVE         EQU     $6000           ; digit select bits for display
SEGMENT_DRIVE       EQU     $7000           ; segment enable bits for display

;;  PAGE 2 DISPLAY HARDWARE DEFINTIONS
SEGA                EQU     ACC2            ; SEGMENT A (TOP)
SEGB                EQU     ACC5            ; SEGMENT B (UPPER RIGHT)
SEGC                EQU     ACC3            ; SEGMENT C (LOWER RIGHT)
SEGD                EQU     ACC7            ; SEGMENT D (BOTTOM)
SEGE                EQU     ACC6            ; SEGMENT E (LOWER LEFT)
SEGF                EQU     ACC4            ; SEGMENT F (UPPER LEFT)
SEGG                EQU     ACC1            ; SEGMENT G (CENTER)
DP                  EQU     ACC0            ; DECIMAL POINT
COLON               EQU     ACC7*$100
SET                 EQU     ACC6*$100
CURR                EQU     ACC5*$100
TODAY               EQU     ACC4*$100
YEST                EQU     ACC3*$100
HIGH                EQU     ACC2*$100
LOW                 EQU     ACC1*$100
MEAN                EQU     ACC0*$100
DEGC                EQU     ACC7
DEGF                EQU     ACC6
AM                  EQU     ACC5
PM                  EQU     ACC4
DATE                EQU     ACC3
ZONE1               EQU     ACC2
ZONE2               EQU     ACC1
ZONE3               EQU     ACC0

A                   EQU     SEGA+SEGB+SEGC+SEGE+SEGF+SEGG   ; LETTER A
B                   EQU     SEGC+SEGD+SEGE+SEGF+SEGG        ; LETTER b
C                   EQU     SEGA+SEGD+SEGE+SEGF             ; LETTER C
D                   EQU     SEGB+SEGC+SEGD+SEGE+SEGG        ; LETTER d
E                   EQU     SEGA+SEGD+SEGE+SEGF+SEGG        ; LETTER E
F                   EQU     SEGA+SEGE+SEGF+SEGG             ; LETTER F
H                   EQU     SEGB+SEGC+SEGE+SEGF+SEGG        ; LETTER H
I                   EQU     SEGB+SEGC                       ; LETTER I
L                   EQU     SEGD+SEGE+SEGF                  ; LETTER L
O                   EQU     SEGA+SEGB+SEGC+SEGD+SEGE+SEGF   ; LETTER O
P                   EQU     SEGA+SEGB+SEGE+SEGF+SEGG        ; LETTER P
S                   EQU     SEGB+SEGC+SEGD+SEGF+SEGG        ; LETTER S
U                   EQU     SEGB+SEGC+SEGD+SEGE+SEGF        ; LETTER U
ZERO                EQU     O                               ; DIGIT 0
ONE                 EQU     I                               ; DIGIT 1
TWO                 EQU     SEGA+SEGB+SEGD+SEGE+SEGG        ; DIGIT 2
THREE               EQU     SEGA+SEGB+SEGC+SEGD+SEGG        ; DIGIT 3
FOUR                EQU     SEGB+SEGC+SEGF+SEGG             ; DIGIT 4
FIVE                EQU     S                               ; DIGIT 5
SIX                 EQU     SEGA+SEGC+SEGD+SEGE+SEGF+SEGG   ; DIGIT 6
SEVEN               EQU     SEGA+SEGB+SEGC                  ; DIGIT 7
EIGHT               EQU     SEGA+SEGB+SEGC+SEGD+SEGE+SEGF+SEGG ; DIGIT 8
NINE                EQU     SEGA+SEGB+SEGC+SEGD+SEGF+SEGG   ; DIGIT 9

;;; PAGE 3 RAM ASSIGNMENTS
    SEG.U                               ; RAM 128 BYTES
    ORG                 $80

RAM_PTR             DS      1
LINE_PHASE          DS      1
TIME_CORRECT        DS      2
DIGIT               DS      1
MSD                 DS      6
ADC_STATE           DS      1
A_D_SELECT          DS      1
CONV_BUF            DS      5*3
START_TIME          DS      2
TIME_RESIDUAL       EQU     CONV_BUF+2
MEAN_COUNT          DS      1
TODAY_MIN1          DS      3
TODAY_MIN2          DS      3
TODAY_MIN3          DS      3
TODAY_MAX1          DS      3
TODAY_MAX2          DS      3
TODAY_MAX3          DS      3
LAST_MIN1           DS      3
LAST_MIN2           DS      3
LAST_MIN3           DS      3
LAST_MAX1           DS      3
LAST_MAX2           DS      3
LAST_MAX3           DS      3
LAST_MEAN1          DS      3
LAST_MEAN2          DS      3
LAST_MEAN3          DS      3
TODAY_MEAN1         DS      3
TODAY_MEAN2         DS      3
TODAY_MEAN3         DS      3
TWENTIETHS          EQU     CONV_BUF+5
TENTHS              EQU     CONV_BUF+8
TIME_OF_DAY         EQU     CONV_BUF+11
DAY                 EQU     LAST_MEAN1+2
MONTH               EQU     LAST_MEAN2+2
YEAR                EQU     CONV_BUF+14
DISP_SELECT         DS      2
ZONE_SELECT         EQU     DISP_SELECT+1
ALT_SELECT          EQU     LAST_MEAN3+2
LAST_KBD            DS      1
ALARM               DS      1
TEMP                DS      4
TEMP_BUF            DS      6

                    SEG     CODE,0
                    ORG     $F800

;;;  PAGE 4 CLOCK TIMER INTERRUPT SERVICING

; All of the software in this program can be divided into two
; categories - that which is executed as the result of an interrupt
; and that which is not. The software which is not executed under
; interrupt includes initialization after reset, keyboard servicing,
; display servicing and serial communications.  These are all basically
; asynchronous operations in that they are not periodic functions
; that are performed at fixed time intervals, although they are 
; indirectly tied into a repetition rate of 20 times each second in
; order to provide sipmle debouncing of the keyboard.
; All other functions of this program are periodic and are performed
; by the timer interrupt service routine. This routine can request
; that a timer interrupt occur, causin the routine to be executed
; at whatever interval it likes, and the interval which it selects is
; 2.778 milliseconds. This value is essentially 1/360 second, and the
; reason that this inverval was chosen was to allow the display
; refreshing to be performed 360 times each second. In addition, it
; will be seen that there are other functions which are performed at
; less frequent intervals by this routine sipmly by executing the 
; appropriate code every n'th time an interrupt occurs, where n is
; 6, 18, 2160, etc. This gives execution frequencies of 60 hz, 20 hz,
; 1/6 hz, once each 10 minutes, once each day, once each
; month and once each year.  For clarity, the software has been
; divided up to show which parts are executed at which intervals.
;
; One of the main functions of this routine is to operate the
; analog to digital converter, or A/A. each conversions consists of
; two simple operations. First, one of the analog input signals is
; used to charge up a capacitor for a fixed time period. This means
; that the amount of charge in the capacitor is now directly
; proportional to the signal voltage. Next, the capacitor is
; discharged at a fixed rate using a reference signal of the
; opposite polarity until the capacitor voltage is back to zero.
; The time taken to discharge the capacitor is measured and is a
; direct measure of the input signal.  The hardware then holds the 
; capacitor in a discharged state until the start of the next 
; measurement cycle.  This process is repeated every 1/20 second,
; with the integration of the input signal taking the first 1/60
; second and the discharge and wait for the next measurement taking
; the remaining 2/60 second.

;;;  PAGE 5 CLOCK TIMER INTERRUPT SERVICING

;;;  Begin the timer interrupt service routine

TIMER_INT           EQU     *               ; Arrive at this point an average of once every
                                            ;  1/360 second as a result of a timer interrupt
                    ; The task which is most sensitive to timing, and is therefore
                    ; performed first, is that of maintaining synchronization with the
                    ; AC line. Once every 1/360 second the line voltage is tested to see
                    ; whether it is above or below zero volts. At any time it is
                    ; possible to check this information for the previous 8 interrupts,
                    ; which covers a period of slightly more than one full line cycle.
                    ; This infomation will be used by another part of this routine,
                    ; which executes once every 1/20 second.
    
                    LDAB    PORT1           ; The line voltage information is read in through the
                                            ;  most significant bit of port 1.
                    LDAA    LINE_PHASE      ; The value of this bit for the last eight interrupts
                                            ;  is contained in the variable LINE_PHASE.
                    LSLD                    ; This instruction throws away the oldest bit line_phase,
                                            ;   shifts the other seven left one place and enters the newest
                                            ;   bit in the least significant bit of the value
                    STAA    LINE_PHASE      ; Save this for later interrupts and other uses

                    ; Every time there is a timer interrupt the hardware must be told
                    ; when the next interrupt is to occur. This information is written
                    ; into a 16 bit hardware register called the output compare register
                    ; (OCR). Since this register already contains the time at which the
                    ; present interrupt was triggered, all that is necessary is to take
                    ; this value and add the equivalent of 1/360 second to it, then
                    ; put it back.
                    LDAA    TIMER_CONTROL   ; Read the flag so that it will be cleared
                    LDD     TIMER_OCR_REG   ; Load the 16 bit time at which this interrupt
                    ADDD    #2778           ;  occurred - the value 2778 is the number of instruction
                                            ;  cycles, at 1,000,000 cycles each second, in 1/360 second
                    STD     TIMER_OCR_REG   ; The value 2778 is not exactly the correct value
                                            ;  but a further correction will be made later to compensate

                    ; The serial port servicing, once it is written, will require a
                    ; short routine to read the serial port receiver register and
                    ; transfer any received character to the input buffer
                        
    
;;; PAGE 6 CLOCK TIMER INTERRUPT SERVICING

                    ; The only other task which occurs every 1/360 second is the
                    ; refreshing of the display, at each interrupt the digit which is 
                    ; currently being displayed is turned off and the required segments
                    ; of the next digit are turned on.

                    CLR     SEGMENT_DRIVE   ; Turn off all segments before enabling the next
                                            ;  digit in order to avoid 'ghost' segments
                    LDAA    DIGIT           ; The variable 'digit' has just one bit set corresponding
                                            ;  to the digit which is to be enabled next
                    TAB                     ; Make a second copy of it
                    ORAB    ALARM           ; Add in the bits which control the two alarm outputs
                    STAB    DIGIT_DRIVE     ; Write these bits out to the hardware
                    LDX     #MSD-1          ; It is now necessary to find out which segments to turn
                                            ;  on for this digit - this information is contained in
                                            ;  a six byte array in memory
SELECT_RFSH         INX                     ; Point to the segments of the first digit
                    LSLA                    ; If that is the one to be displayed then its bit will shift out
                                            ;  of register A, leaving a zero
                    BNE     SELECT_RFSH     ; If the value in A is not yet zero then it is not
                                            ;  the first digit, so go back, point to the second digit
                                            ;  and see if it is the one to be displayed, etc.
                    LDAA    0,X             ; Eventually the bit which was set in register A will be
                                            ;  shifted out and the index register will contain the address
                                            ;  of the segments for this digit, so load the segment bits
                                            ;  themselves into register A
                    STAA    SEGMENT_DRIVE   ; Write them out to the hardware, turning on
                                            ;  those segments
                    LSL     DIGIT           ; Shift the digit bit left one place so that when the next
                                            ;  interrupt occurs the next digit will be displayed
                    BEQ     SIXTY_HZ        ; Eventually the bit will be shifted out to leave a 
                                            ;  value of zero - each time this occurs we go on to
                                            ;  execute the 60 Hz and slower operations - the other
                                            ;  five times out of six we just make sure that an 
                                            ;  output bit will get set to zero
                    LDAA    #TIMER_EOCI     ; Clear the bit that controls the level of the 
                    STAA    TIMER_CONTROL   ;  output compare pin - this guarantees that it 
                                            ;  will normally be low
                    RTI                     ; This completes the functions which are performed 360 times
                                            ;  each second                    

;; PAGE 7 CLOCK TIMER INTERRUPT SERVICING

; Operations which are performed 60 times each second

SIXTY_HZ            LDAA    #ACC2           ; Each time the digit bit shifts all the way out, must
                    STAA    DIGIT           ;  reinitialize it to select last digit

                    ; The only other 60 Hz operation is the selection of which 20 Hz
                    ; operation is to be performed at this time
                    LDAA    ADC_STATE       ; The ADC state counter can have values 0, 1, 2
                                            ;  or 3, but this routine should only see values of 1, 2, or 3 at this
                                            ;  point
                    DECA                    ; Decrement the state value to 0, 1, or 2 and save it
                    STAA    ADC_STATE       ; The value of 0 is used as an indicator to the 
                                            ;  background task that another 1/20 second has elapsed - the
                                            ;  background task will then set the state value back up to 3
                    ; Now branch to one of three routines which are each executed
                    ; at 20 hz
                    BEQ     STATE_0         ; If the state counter is zero, execute the state zero
                                            ;  routine
                    DECA                    ; Otherwise see if it is one
                    BEQ     STATE_1         ; Go to state one if it is
                    DECA                    ; Finally, see if we are in state 2
                    BEQ     STATE_2
                    CLR     ADC_STATE       ; If it is noe of these then there is a very serious
                    RTI                     ;  problem, but see if we can recover from it by just setting
                                            ;  the state to a known value and returning to background

;; PAGE 8 CLOCK TIMER INTERRUPT SERVICING

; A/D servicing
; 
; The three A/D service routines perform the following functions:
; in state one, the integration of one of the anlog signals is
; initiated by writing its selection code into the first latch of
; the A/D circuit. The output level bit is then set high so that 
; there will be a positive transition on the next timer interrupt,
; which will transfer the select bits to the second latch. Before
; any of this is done, however, this routine finishes the previous 
; conversion by reading in the input capture value and performing
; filtering on it.
; In state zero, the integration is terminated and discharging
; begins.
; In state two, nothing is done which involves the A/D, but in
; this state any periodic function which occurs less frequently
; than 20 Hz takes place, such as updating the clock and calendar
; as well as minimum, maximum and mean temperature calculations.
; Phase adjustment to the AC line also takes place in this routine.

; A/D converion, state 1
STATE_1             LDAB    A_D_SELECT      ; See which signal was just measured
                    LDX     #CONV_BUF-3     ; Point to the buffer of filtered conversion values
                    ABX                     ; The filtered values are each three bytes apart so add the
                    LSLB                    ;  channel number times three to point to the appriopriate entry
                    ABX                     ; This is the address of the filtered value for this channel
                    LDD     TIMER_ICR_REG   ; Get the time at which the conversion ended
                    SUBD    START_TIME      ; Subtract the time at which the discharge started
                                            ; This is the discharge time and is a direct measure
                                            ;  of the signal voltage
                    SUBD    0,X             ; Find the difference between this and the old filtered
                                            ;  value - the object here is to add back a small fraction
                                            ;  of this difference to the old filtered value in order
                                            ;  to arrive at a new filtered value - in this case we will 
                                            ;  take 1/4 of the difference and add it back
                    ASRA                    ; Do an arithmetic shift right since the difference is signed
                    RORB                    ; Shift the second byte
                    ASRA                    ; Shift a second time
                    RORB
                    ADDD    0,X             ; Add the upper two bytes of the values
                    STD     0,X             ; Save these two bytes
; The net result of the above is as follows: at each conversion the
; old filtered value has added to it 1/4 of the differences between 
; the new conversion value and the old filtered value. This causes
; the filter response to be expoential with a time constant of
; 4 times the interval between conversions. Since one conversion
; is performed every 1/20 second and there are five channels, each
; channel is measured once every 0.25 seconds. This makes the time
; constant of the filter equal to 0.25 * 4 = 1 second.
                    LDAA    A_D_SELECT      ; Now that the conversion is complete,
                    DECA                    ;  see which input to measure next
                    BNE     STORE_A_D_S     ; As long as the channel number is between 1 and 5, use it
                    LDAA    #5              ; When it decrements to zero, restart it at 5
STORE_A_D_S         STAA    A_D_SELECT      ; Save the new counter value
SET_MUX             STAA    MUX             ; Set the multiplexer to select this channel on the
                                            ;  next output compare interrupt
                    LDAA    #TIMER_EOCI+TIMER_OLVL ; Must also set the output level control
                    STAA    TIMER_CONTROL   ;  bit to cause a positive transition at that                    
                                            ;  interrupt
                    RTI                     ; This completes state 1 processing

;; PAGE 10 CLOCK TIMER INTERRUPT SERVICING

; A/D conversion state 0
STATE_0             EQU     *
                    LDD     TIMER_OCR_REG   ; Note the time at which the discharge will begin
                    STD     START_TIME      ; Save it to use in the calculations for this conversion
                    LDAA    #7+ACC3         ; To start the discharge, select channel 7 of the
                                            ;  multiplexer and also set the bit which enables the compartor
                    BRA     SET_MUX         ; Use the section of code above to write this value out
                                            ;  to the multiplexer channel select latch and also to prepare
                                            ;  the output compare pin for a positive transition on the next
                                            ;  interrupt
; A/D conversion state 2
STATE_2             EQU     *               ; This routine is not reuqired to perform any A/D
                                            ;  related functions, but it does perform every periodic
                                            ;  function whose period is longer than 1/20 second as well as
                                            ;  maintaining synchronization with the line

;; Line synchronization function

; There are two basic purposes to this section: first, it maintains
; correct synchronization with the ac line at all times. To do
; this, it checks once during each conversion to see whether then 
; internal timing has shifted by more than plus or minus 60 degrees
; with respect to the ac line. If it has then the internal timing
; is shifted slightly, making that measurement cycle slightly longer
; or slightly shorter than nominal to try to bring the timing back 
; within the plus or minus 60 degree phase window. If there has been
; a large phase shift then it might take several shortened or 
; lengthened cycles to get back to within the window.  As soon as they 
; are back in phase, normal length cycles are resumed. The second
; function is more subtle. It is also activated by phase shifts of
; more than 60 degrees, but in this case the effect is to make very
; small but permanent changes in the normal lengt of a cycle.  This
; type of change would not be necessary if the ac line signal were
; always going to be present to maintian synchronization, but since
; the instrument can have a battery backup, and since better accuracy 
; than just the nominal crystal frequency accuracy is desirable, the
; software is, in effect, gradually forming a correction to the
; crystal frequency, based on line frequency, which will then be 
; used while the ac power is off.

                    LDAA    TIME_CORRECT+1  ; Since the period of a conversion is 50,000
                                            ;  instruction cycles, if the correction were an integer value in
                                            ;  cycles, the best correction that could be made is plus or minus
                                            ;  1/50,000 which is nearly two seconds per day of error - to get
                                            ;  around this, a one byte fractional correction is maintained.
                    ADDA    TIME_RESIDUAL   ; To use this correction, it is added each time to 
                                            ;  a one byte residual value, and each time there is a carry
                                            ;  from this byte, one more full instruction cycle is added
                                            ;  to this time
                    STAA    TIME_RESIDUAL   ; Save the new residual
                    LDD     TIMER_OCR_REG   ; Take the previously calculated time of the next
                                            ;  interrupt
;; PAGE 11          
                    ADCB    TIME_CORRECT    ; Add in the upper byte of the correction value
                    ADCA    #0              ; Handle any carry to the top byte of the interrupt time
                    TST     TIME_CORRECT    ; See whether the correction was intended to be 
                                            ;  positive or negative
                    BPL     IS_POS          ; If it is negative then
                    DECA                    ; subtract one count from the top byte
IS_POS              STD     TIMER_OCR_REG   ; Put back the corrected time of the next interrupt
                    LDX     TIME_CORRECT    ; Get the correction value in case it whill need to
                                            ;  be adjusted
                    LDAA    LINE_PHASE      ; See what the AC line has been doing for the last
                                            ;  eight interrupts
                    CMPA    #%00011100      ; Test for internal time fast by 60 degrees or more
                    BNE     IS_NOT_FAST     ; If it is then
                    DEC     TIMER_OCR_REG   ;  shorten this conversion
                    DEX                     ;  and slightly shorten all subsequenty conversions
IS_NOT_FAST         CMPA    #%11100011      ; Else test for internal time slow
                    BNE     IS_NOT_SLOW     ; If it is then
                    INC     TIMER_OCR_REG   ;  lengthen this conversion
                    INX                     ;  and slightly length all subsequent conversions
IS_NOT_SLOW         STX     TIME_CORRECT    ; Put back the (possibly) modified correction

;; PAGE 12
                    
; The final operation to be performed at 20 Hz is decrementing a
; counter which is used to time 6 second intervals, because the
; next groups of functions is performed ten times each minute
                    DEC     TWENTIETHS      ; This variable counts twentieths of a second
                    BNE     IRET            ; If six seconds have not elapsed yet then just return
                    LDAA    #120            ; Else restart the counter at six seconds
                    STAA    TWENTIETHS   
                    LDAA    TENTHS          ; The next counter is used to count tenths of a minute.
                    ADDA    #1              ;  In BCD, up to 99 tenths
                    DAA                     ; Since this is a BCD counter, adjust it to a decimal value each
                    STAA    TENTHS          ;  time it is incremented
                    BEQ     TEN_MINUTES     ; If ten minutes has elapsed then perform those
                                            ;  functions which are done every ten minutes.

;; PAGE 13

; The next function is performed 99 times every ten minutes.  It is
; not done the hundredth time because there is a lot more to be
; done that time and it would make the processor loading much higher
; to do both. the function of this section of code is to update the
; minimum and maximum values of each of the three zone temperatures.
; The mean values are updated at ten minute intervals.
                    LDAA    TIME_OF_DAY     ; Load the current time into register A in case we
                                            ;  need to note the time at which a new min or max was reached
                    LDX     CONV_BUF        ; Load the current temperature of zone 1
                    CPX     TODAY_MAX1      ; see if it is a new daily maximum
                    BHS     IS_HIGHER1      ; Since the conversion value varies inversely with the
                                            ;  temperature, the sense of the test looks backward
                    STX     TODAY_MAX1      ; If the current temp ois higher than the previous
                                            ;  daily maximum then replace the old maximum with the current
                    STAA    TODAY_MAX1+2    ; The third byte of this value is the time of day
                                            ;  at which the maximum occured, in tens of minutes.
IS_HIGHER1          CPX     TODAY_MIN1      ; If no new maximum then see if new minimum
                    BLS     IS_LOWER1       ; Again, skip the next 2 instructions if no new min
                    STX     TODAY_MIN1
                    STAA    TODAY_MIN1+2
IS_LOWER1           LDX     CONV_BUF+3      ; Do the same for zone 2
                    CPX     TODAY_MAX2
                    BHS     IS_HIGHER2
                    STX     TODAY_MAX2
                    STAA    TODAY_MAX2+2
IS_HIGHER2          CPX     TODAY_MIN2
                    BLS     IS_LOWER2
                    STX     TODAY_MIN2
                    STAA    TODAY_MIN2+2
IS_LOWER2           LDX     CONV_BUF+6      ; And for zone 3
                    CPX     TODAY_MAX3
                    BHS     IS_HIGHER3
                    STX     TODAY_MAX3
                    STAA    TODAY_MAX3+2
IS_HIGHER3          CPX     TODAY_MIN3
                    BLS     IS_LOWER3
                    STX     TODAY_MIN3
                    STAA    TODAY_MIN3+2
IS_LOWER3           CLR     ALARM           ; Calculate the new alarm bits, first clearing out the old
                    LDD     CONV_BUF+12     ; See if zone 3 is higher than the high reference
                    SUBD    CONV_BUF+6
                    ROL     ALARM           ; If it is then set the low temp alarm bit (high to turn on)
                    LDD     CONV_BUF+6      ; See if zone 3 is lower than the average of the high
                    LSLD                    ;  and low references
                    SUBD    CONV_BUF+9
                    SUBD    CONV_BUF+12
                    ROL     ALARM           ; If it is then set the high temp alarm (high to turn on)
IRET                RTI                     ; This completes the operations performed ten times each
                                            ;  minute
;; PAGE 14 
;; Functions performed every ten minutes

; Every ten minutes the calculated value of today's mean temperature
; for each of the three zones is updated. After that, a check is made
; for two special times of the day, midnight and 2AM.

TEN_MINUTES         EQU     *
                    LDD     TODAY_MEAN1+1   ; Get the second and third bytes of the running 
                                            ;  sum of zone 1 at ten minute intervals so far today
                    ADDD    CONV_BUF        ; Add in the latest value for this zone
                    STD     TODAY_MEAN1+1   ; Return the updated second and third bytes
                    BCC     NO_CARRY1       ; If there was a carry generated by this addition then
                    INC     TODAY_MEAN1     ;  increment the first byte

NO_CARRY1           LDD     TODAY_MEAN2+1   ; Repeat the process for zone 2
                    ADDD    CONV_BUF+3
                    STD     TODAY_MEAN2+1
                    BCC     NO_CARRY2
                    INC     TODAY_MEAN2

NO_CARRY2           LDD     TODAY_MEAN3+1   ; And for zone 3
                    ADDD    CONV_BUF+6
                    STD     TODAY_MEAN3+1
                    BCC     NO_CARRY3
                    INC     TODAY_MEAN3

NO_CARRY3           INC     MEAN_COUNT      ; Increment the counter to show that one more value has
                                            ;  been summed in
                    LDAA    TIME_OF_DAY     ; Now increment the time-of-day counter by ten minutes
                    INCA
                    CMPA    #6*24           ; See if it is midnight
                    BHS     MIDNIGHT        ; If it isi then take care of any daily chores
                    STAA    TIME_OF_DAY     ; Otherwise save the incremented time
                    CMPA    #6*2            ; Now see if it is 2AM
                    BNE     IRET            ; If not then we are done for now
                    LDAA    PORT1           ; If it is 2AM then there is a possibility that we need to
                    BITA    #DST_JUMPER     ;  adjust for daylight saving time
                    BNE     IRET            ; If the jumper has been removed then daylight saving time is
                                            ;  disabled.
                    LDAA    MONTH           ; Next see if it is April or October
                    CMPA    #10             ; October is month number 10
                    BEQ     OCTOBER         ; If it is October then check for return to standard time
                    CMPA    #4              ; Otherwise see if it is April
                    BNE     IRET            ; If it is not April either there is nothing more to do
                    LDAA    DAY             ; Now see what day of the month it is
                    LSRA                    ; Shift out the daylight savings flag
                    SUBA    #24             ; See if this is last week in April
                    BLO     IRET            ; If not then skip it
; Note - April 24 1984 was a Tuesday
                    ADDA    #2              ; Add 2 to get the day of week this would have been in 1984
                    BSR     DAY_OF_WEEK     ; Now find out what day of the week it is this year
                    BNE     IRET            ; If it is not Sunday (for which zero is returned), skip it
                    LDAA    TIME_OF_DAY     ; Only if this is the last Sunday in April do we
                    ADDA    #6              ;  add one hour to the time
                    STAA    TIME_OF_DAY
                    RTI

;; PAGE 15

OCTOBER             LDAA    DAY             ; If this is October then see what the date is
                    LSRA                    ; Shift out the daylight saving flag
                    BCS     IRET            ; If this bit is set then we have already adjusted then time
                                            ;  back to standard time
                    SUBA    #25             ; Otherwise see if this is the last week in October
                    BLO     IRET            ; If not then skip it
; Note - October 25 1984 was a Thursday
                    ADDA    #4              ; Add 4 to get the day of week this would have been in 1984
                    BSR     DAY_OF_WEEK     ; Now fine out which day of the week it is this year
                    BNE     IRET            ; If it is not Sunday (for which zero is returned), skip it
                    LDAA    TIME_OF_DAY     ; Only if this is the last Sunday in October do we
                    SUBA    #6              ;  subtract one hour from the time
                    STAA    TIME_OF_DAY
                    INC     DAY             ; Set the daylight saving time bit to prevent changing the
                                            ;  time again in another hour
                    RTI

DAY_OF_WEEK         EQU     *               ; Subroutine to correct the day of the week according
                                            ;  to the year
                                            ; Pass a value from 0 to 6 in A, returns a value from 0 to 6 in A
                    LDAB    YEAR            ; Get the value of the year, relative to 1984
                    ANDB    #ACC0+ACC1      ; Get the value mod 4
                    ABA                     ; Add one day for each non leap year past the last leap year
                    LDAB    YEAR            ; Get the year again
                    LSRB                    ; look at just the other six bits in the year
                    LSRB
HUNDRED_YEAR        CMPB    #(2100-1984)/4  ; See if the year is greater than or 
                                            ;  or equal to 2100
                    BLO     LEAP_YEARS      ; If not then finish correcting leap years
                    ADDA    #5              ; In a normal period of 100 years there are 76 non leap years
                                            ;  and 24 leap years, for a total shift of 124 days
                                            ;  124 mod 7 is 5
                    SUBB    #100/4          ; This correction is not made for the 100 year period
                                            ;  which includes the year 2000, since it is a leap year
                    BRA     HUNDRED_YEAR    ; Repeat the above in case the year is 2200 or later

LEAP_YEARS          PSHA                    ; Save the day count on the stack
                    LDAA    #5              ; multiply the number of leap years by 5
                    MUL
                    PULA                    ; Get back the day couint
                    ABA                     ; Combine them
DEDUCT_7            SUBA    #7              ; Subtract one week from the day count
                    BHI     DEDUCT_7        ; Do this over and over until the count goes negative
                                            ;  or to zero (it will only go to zero for a Sunday)
                    RTS                     ; Return with the day-of-week value from -6 to 0 and its
                                            ;  Status in the flags

;; PAGE 16

;; Functions performed at midnight every day

; The following operations all have to do with updating the day,
; month and year counters.  The day counter has a range of one to
; 28, 29, 30 or 31, depending on the month, but is stored as twice
; this value so that the least significant bit can be used as a flag
; to indicate that a change from daylight saving time to standard
; time has been made that day. The month has a value from 1 to 12
; (January to December) and the year has a value between 0 (1984)
; and 255 (2239).  All corrections for leap years are made properly
; over this entire time span.

MIDNIGHT            EQU     *
                    BSR     DAYS_IN_MONTH   ; Before incrementing the day we must see how many
                                            ;  days there are in this month.
                                            ; This value will be returned in register B
                    LDAA    DAY             ; Now see what day of the month this is
                    LSRA                    ; Shift out the daylight saving flag
                    CBA                     ; See if we just ended the last day of a month
                    BLO     INC_DAY         ; If not then just increment the day normally
                    LDAA    MONTH           ; If the month has just ended then increment the month
                    CMPA    #12             ; When the month ends there is also the possibility that
                    BLO     INC_MONTH       ;  the year has just ended
                    INC     YEAR            ; Execute this instruction once each year at midnight on
                                            ;  December 31

;                   BEQ     *
; Wouldn't it be fun to actually put the above instruction into the 
; program? I can just see it now......
; It is new years eve in the year 2239, and precisely at midnight,
; all around the world, everyone's HP clock thermometer suddenly
; goes dead! Since every copy of this listing has long since been
; lost, it will remain a mystery forever how this couild have happened.
;
; But seriously, what do we do when the last counter finally runs out?
; Do we simply set it become 1984 all over again? This would allow the
; corrections for leap year to be done properly for another 60 years
; until the year 2300. But what about the corrections for daylight
; saving time? The best soluition is to find a year between 1984 and
; 2239 which is the same as 2240 with respect to leap years and 
; days of the week. As it happens, there are many years which meet
; these requirements, the earliest being 1992. Of course, the year
; will read incorrectly and it will still fail in the year 2300, but
; then you can't have everything.
                    BNE     NOT_ROLLOVER    ; In any year except 2240, skip the next two instructions
                    LDAA    #1992-1984      ; Reset the year to 1992
                    STAA    YEAR
NOT_ROLLOVER        CLRA                    ; Reset the month to January
INC_MONTH           INCA                    ; Increment the month value
                    STAA    MONTH           ; Save the incremented value
                    CLRA                    ; Start the day counter over at day 1
INC_DAY             INCA                    ; Increment the day value
                    LSLA                    ; Save it doubled form to leave room for the DST flag
                    STAA    DAY             ; Save the incremented value

;; PAGE 17  CLOCK TIMER INTERRUPT SERVICING

; The last function to be performed by the clock interrupt is
; the daily calculation of the average temperatures for that day and
; the saving of those values as the previous day's averages, followed
; by the re-initialization of today's averages and the updating of
; all of the minimum and maximum values

                    LDX     #TODAY_MEAN1    ; Point to the sum of the values for zone 1
                    BSR     AVERAGE         ; Convert this to an average value
                    STD     LAST_MEAN1      ; Save this as yesterday's average
                    LDX     #TODAY_MEAN2    ; Do the same for the other two zones
                    BSR     AVERAGE
                    STD     LAST_MEAN2
                    LDX     #TODAY_MEAN3
                    BSR     AVERAGE
                    STD     LAST_MEAN3
                    LDX     #LAST_MIN1-TODAY_MIN1 ; Now set yesterday's min and max values
                                            ;  to today's - use X as a byte counter and also a pointer
SET_LAST            LDAA    TODAY_MIN1-1,X  ; Get one byte of today's data
                    STAA    LAST_MIN1-1,X   ; Copy it into the corresponding byte of
                    DEX                     ;  yesterday's data and decrement the pointer/counter
                    BNE     SET_LAST        ; Repeat until all bytes have been copied
                    CLR     TIME_OF_DAY     ; Reset the current time of day to midnight
                    BSR     INIT_MINMAX     ; Initialize the minimu, maximum and average values
IRQ_EXIT            RTI

;; PAGE 18 CLOCK TIMER INTERRUPT SERVICING

; Subroutine to find out how many days there are in a month
DAYS_IN_MONTH       EQU     *
                    LDAB    MONTH           ; See which month this is
                    LDX     #MONTH_TABLE-1  ; Point to a lookup table of days in a month
                    ABX                     ; Index into the lookup table
                    LDAB    0,X             ; Load the value from the table
                    CMPB    #28             ; See if this is February
                    BNE     RETURN_DAYS     ; If it is not then this is the correct number of days
                    LDAA    YEAR            ; If it is then we must also see if this is a leap year
                    BITA    #ACC0+ACC1      ; By looking at just the last two bits in the year we
                    BNE     RETURN_DAYS     ;  know whether or not it is divisible by four
                    CMPA    #2100-1984      ; If it is then we must also test for two such years
                    BEQ     RETURN_DAYS     ;  which will not be leap years - 2100 and 2200
                    CMPA    #2200-1984      ; Oddly enough, the year 2000 is a leap year
                    BEQ     RETURN_DAYS     ; After eliminating all non leap years over this time
                    INCB                    ;  span, this leaves only leap years, for which February
                                            ;  has 29 days
RETURN_DAYS         RTS                     ; Return with the number of days in the current month
                                            ;  in register B

MONTH_TABLE         EQU     *               ; Lookup table of days in the month
                    DC      31              ; January
                    DC      28              ; February (may be 29)
                    DC      31              ; March
                    DC      30              ; April
                    DC      31              ; May
                    DC      30              ; June
                    DC      31              ; July
                    DC      31              ; August
                    DC      30              ; September
                    DC      31              ; October
                    DC      30              ; November
                    DC      31              ; December

;; PAGE 19 - CLOCK TIMER INTERRUPT SERVICING

; Subroutine to calculate the average value from the sum of the
; values and the number of values summed.
AVERAGE             EQU     *
                    LDAA    #17             ; Go through the divide loop 17 times but only save 16 bits
                                            ;  of quotient
                    PSHA                    ; Push the counter byte onto the stack
                    LDAA    2,X             ; X points to the sum, so 2,x is the third byte of the sum
                    PSHA                    ; Use another stack location for this byte
                    LDD     0,X             ; Now load the first two bytes of the sum
                    TSX                     ; Make X now point to the two bytes which were placed on the stack
                    CLC                     ; Clear the carry to prevent A 1 from getting shifted in below
AVG_DIVIDE          ROL     0,X             ; Shift the quotient and remainder left one bit
                    ROLB                    ; The lowest of these three bytes is on the stack, then other
                    ROLA                    ;  two in B and A                    
                    BCS     SUBTRACT        ; If a non-zero bit of the remainder shifts out the top
                                            ;  then force a subtraction
                    CMPA    MEAN_COUNT      ; Otherwise decide whether or not to subtract based
                    BLO     NO_SUBTRACT     ;  on whther the remainder is greater or less than
                                            ;  the number being divided by, the number of summed values
SUBTRACT            SUBA    MEAN_COUNT      ; Every time the remainder is large enough,
                    CLC                     ;  subtract the divisor and shift in a zero for the quotient
NO_SUBTRACT         EQU     *               ; Every time it is too small, shift in a one
                    DEC     1,X             ; See if 17 passes have been made through the above loop
                    BNE     AVG_DIVIDE      ; Repeat until done
; After 17 passes, the 16 bit quotient is in B and the top byte of
; the stack, in complemented form, and the seventeenth bit of the 
; quotient, also complemented, is in the carry flag
                    TBA                     ; Move the first byte of quotient to register A
                    PULB                    ; Move the second byte to B
                    INS                     ; Discard the counter from the stack
                    BCS     NO_CARRY        ; Use the next bit of the quotient for rounding
                    SUBD    #1              ; At this point all bits are still complemented so the
                                            ;  'branch if carry clear' and 'increment double' instructions
                                            ;  are instead 'branch if carry set' and 'decrement double'
NO_CARRY            COMA                    ; Finally convert the rounded qoutient from complemented to
                    COMB                    ;  true form and return
                    RTS
                    
;; PAGE 20 CLOCK TIMER INTERRUPT SERVICING

; Subroutine to initialize today's minimum, maximum, and mean values

INIT_MINMAX         EQU     *
                    LDD     CONV_BUF        ; Take the current values of the zones
                    STD     TODAY_MEAN1+1   ; Initialize the sum, the minimum, and the maximum
                    STD     TODAY_MIN1      ;  to this value
                    STD     TODAY_MAX1
                    LDD     CONV_BUF+3
                    STD     TODAY_MEAN2+1
                    STD     TODAY_MIN2
                    STD     TODAY_MAX2
                    LDD     CONV_BUF+6
                    STD     TODAY_MEAN3+1
                    STD     TODAY_MIN3
                    STD     TODAY_MAX3
                    LDAA    TIME_OF_DAY     ; Initialize the time of the min and max of each zone
                                            ;  to the current time
                    STAA    TODAY_MIN1+2
                    STAA    TODAY_MAX1+2
                    STAA    TODAY_MIN2+2
                    STAA    TODAY_MAX2+2
                    STAA    TODAY_MIN3+2
                    STAA    TODAY_MAX3+2
                    CLRA                    ; Initialize the highest byte of each sum to zero
                    STAA    TODAY_MEAN1
                    STAA    TODAY_MEAN2
                    STAA    TODAY_MEAN3
                    INCA                    ; Set the number of values that have been summed so far this
                    STAA    MEAN_COUNT      ;  to one
                    RTS

;; PAGE 21 DISPLAY MODE ROUTINES
               
; Main Program     
; The main program consists basically of four mode routines. The 
; primary mode routine is the display routine. The one in which each 
; of the pieces of information is individually displayed and selected
; by processing the three buttons.  Next there is the alternate display
; routine in which several pices of information are displayed
; alternately over a period of several seconds. Then there is the
; set mode in which setpoints are entered for several parameters,
; such as the time and date.  Finally, there is a diagnostic mode in
; which information aobut the functioning of the hardware itself
; can be displayed. To go from the display routine to any of the 
; others requires that two keys be pressed simultaneously.

; DISPLAY ROUTINE

DISPLAY_MODE        EQU     *               ; This is the main mode of operation of this
                                            ;  instrument. It is used to display any of 68 different pieces of
                                            ;  information one at a time.  The information is selected by pressing
                                            ;  the tree front panel switches one at a time to specify three
                                            ;  identifiers of the information to be displayed. First there is
                                            ;  switch 1, which specifies whether the information is a centegrade
                                            ;  temperature, fahrenheit temperature, a time of date or a date.
                                            ;  Next, switch 2 specifies whether the information refers to
                                            ;  temperature zones 1, 2 or 3. Finally, switch 3 specifies whether
                                            ;  the information is the current value, the maximum or minimum
                                            ;  value so far today, the mean value so far today, or yesterday's
                                            ;  maximum, minimum or mean.
                    LDD     DISP_SELECT     ; Get the 2 byte value which specifies the
                                            ;  information to be displayed
                    JSR     BACKGROUND      ; Format and display the information
                                            ; This major routine does much more than just format display data
                                            ;  and read the keyboard, which is the last thing it does
                                            ;  before returning
                    LSRA                    ; The keyboard is returned in the low 3 bits of A
                    BCS     INC_SW1         ; If bit 0 was set then switch 1 was just pressed
                    LSRA                    ; If switch 1 was not just pressed then see if switch 2 was
                    BCS     INC_SW2         ; If bit 1 was set then switch 2 was just pressed
                    BEQ     DISPLAY_MODE    ; If switches 1 and 2 were not pressed and bit 2 
                                            ;  is also zero then none of the switches was pressed, so 
                                            ;  just continue to display the same information
; If bit 2 is set then increment the switch 3 selection bits

; PAGE 22 DISPLAY MODE ROUTINES

; Increment the switch 3 selection bitts
                    LDAA    DISP_SELECT     ; This byte contains the selection bits for both
                                            ;  switch 1 and switch 3
                    ADDA    #ACC2           ; Leave the two low bits alone and increment the next 3 bits
                    CMPA    #7*ACC2         ; See if these bits have reached a value of 7
                    BLO     NOT7            ; If they have then
                    ANDA    #ACC0+ACC1      ;  start them over again at zero
NOT7                ANDA    #$FF-ACC1       ; If the mode is time or date then set it to C or F
                    STAA    DISP_SELECT     ; Return the selection bits
                    BRA     DISPLAY_MODE    ; Repeat the loop with the new select bits

; Increment the switch 2 selection bits
INC_SW2             EQU     *               ; Increment the zone selection bits
                    LDAB    ZONE_SELECT     ; Get the current value of these bits
                    INCB                    ; Increment to the next value
                    CMPB    #2              ; See if we have incremented past the last zone
                    BLS     IS_LESS         ; If we have then
                    CLRB                    ;  start over at zone 1
IS_LESS             STAB    ZONE_SELECT     ; Save the new value
                    BRA     DISPLAY_MODE    ; Repeat the loop with the new select bits

; Increment the switch 1 selection bits
INC_SW1             EQU     *               ; Increment the data type selection bits
                    LDAB    DISP_SELECT     ; Get the current value of these bits
SKIP_MODE           INCB                    ; Increment to the next value
                    BITB    #ACC0+ACC1      ; See if they have been incremented to zero
                    BEQ     TO_C            ; If they have then go cancel out the carry into the upper bits
                    BITB    #ACC1           ; If not zero then are they 01 (Fahrenheit)
                    BEQ     TO_F            ; If so then just save the new value
                    BITB    #$FF-ACC0-ACC1  ; See if we are reading current value
                    BEQ     TO_F            ; If we are then this is legal for both date and time
                    BITB    #ACC0           ; If not current then see if we are set to date
                    BNE     SKIP_MODE       ; If so then the combination is illegal, go to the next
                    CMPB    #3*ACC2+2       ; For time the only illegal combinations are today's
                    BEQ     SKIP_MODE       ;  mean and yesterday's mean
                    CMPB    #6*ACC2+2       
                    BEQ     SKIP_MODE
                    BRA     TO_F            ; Otherwise save this value as is

TO_C                SUBB    #4              ; Cancel the carry from the low bits
TO_F                STAB    DISP_SELECT     ; Save the new value of the bits
                    BRA     DISPLAY_MODE    ; Repeat the loop with the new select bits

;; PAGE 23 DISPLAY MODE ROUTINES

; Alternate display routine
ALT_MODE            EQU     *               ; This is a display mode in which two or more pieces
                                            ;  of information can be displayed in sequence at intervals of a few
                                            ;  seconds. The mode is initially entered by pressing both S1 and S2
                                            ;  simultaneously. While in this mode the action of the three switches
                                            ;  is modified. S1 is now used to step more quickly through the
                                            ;  display sequence, or to hold the display on a particular value by
                                            ;  holding the switch in.  S2 is used to select a different zone to
                                            ;  be displayed while on either degrees C or degrees F. S3 is used
                                            ;  to exit from this mode and return to normal display mode.
                    LDD     DISP_SELECT     ; Display the currently selected item
                    JSR     BACKGROUND      ;  and read the keyboard
                    LSRA                    ; See if switch 1 was just pressed
                    BCS     ADVANCE_ALT     ; If it was then advance to the next display item
                    LSRA                    ; See if switch 2 was just pressed
                    BCS     ALT_ZONE        ; If it was then advance to the next zone
                    LSRA                    ; See if switch 3 was just pressed
                    BCS     DISPLAY_MODE    ; If it was then exit from this mode
                    BITB    #ACC0           ; If nothing just pressed then is switch 1 being held in
                    BNE     ALT_MODE        ; If it is then hold the display on this item
ADVANCE_YET         LDAA    TWENTIETHS      ; See if it is time to advance the item
                    CMPA    #49             
                    BEQ     ADVANCE_ALT
                    CMPA    #109
                    BNE     ALT_MODE
ADVANCE_ALT         EQU     *               ; Three seconds have elapsed
                    LDAA    DISP_SELECT     ; Get the corrent select bits
                    INCA                    ; Advance to the next item
VALIDATE_ALT        ANDA    #3              ; Allow current values only
                    STAA    DISP_SELECT     ; Put back the new select bits
                    LDAB    ALT_SELECT      ; See which items are selected for alternate display
                    LSLB                    ; only the low 4 bits are used, one for each item
                    LSLB
                    LSLB
                    LSLB
                    BEQ     BAD_ALT         ; If none of these bits is set then set some, so that we
                                            ;  cannot accidentally get hung up here forever
TEST_ALT            LSLB                    ; Shift the first select bit into the carry
                    DECA                    ; See if this is the right bit to test
                    BPL     TEST_ALT        ; No, not yet - look at the next bit
                    BCC     ADVANCE_ALT     ; If this item is not enabled then try the next one
                    BRA     ALT_MODE        ; Otherwise go ahead and display this item

ENTER_ALT           LDAA    DISP_SELECT     ; Enter this mode here to immediately start
                    BRA     VALIDATE_ALT    ;  displaying a valid mode

;; PAGE 24 DISPLAY MODE ROUTINES

; Select the next zone while in alternate display mode
ALT_ZONE            LDAA    ZONE_SELECT     ; Select the next mode
                    INCA
                    CMPA    #3              ; See if we have passed zone 3
                    BLO     NOT_THREE
                    CLRA                    ; If so then drop back to zone 1
NOT_THREE           STAA    ZONE_SELECT
                    BRA     ADVANCE_YET     ; Now execute the remainder of the above routine

BAD_ALT             DEC     ALT_SELECT      ; If noe of the select bits were set then
                                            ;  set them all
                    BRA     ALT_MODE        ; Go ahead and display the current one

;; PAGE 25 DISPLAY MODE ROUTINES

; Set time and date routine
SET_MODE            EQU     *               ; This is a display mode in which the time and date
                                            ;  settings can be adjusted.  The mode is initially entered by pressing
                                            ;  both s1 and s3 simultaneiously. Whiel in this mode S1 selects the
                                            ;  next setting to be adjusted, S2 increments its value and S3 
                                            ;  decrements it.  After the last setting has been displayed and
                                            ;  possibly adjusted, pressing s1 one more time returns to normal
                                            ;  display mode.
                    LDD     #$070B          ; Special select value to display the year and suppress 
                                            ;  other indicator lights
                    JSR     BACKGROUND      ; Display the current year setpoint and read the keyboard
                    LSRA                    ; See if switch 1 was pressed
                    BCS     SET_MONTH       ; If it was then leave the year and on to the month
                    LSRA                    ; See if switch 2 was pressed
                    BCS     INC_YEAR        ; If it was then increment the year
                    LSRA                    ; See if switch 3 was pressed
                    BCC     SET_MODE        ; If none pressed, stay in this mode
                    DEC     YEAR            ; If switch 3 was pressed the decrement the year
                    BRA     SET_MODE        ; Stay in this mode

INC_YEAR            INC     YEAR            ; If switch 2 was pressed then increment the year
                    BRA     SET_MODE        ; Stay in this mode

SET_MONTH           EQU     *               ; Once the year is set, display and adjust the month
                    LDD     #$033B          ; Special select value to display the month and suppress
                                            ;  the day and indicator lights
                    JSR     BACKGROUND      ; Display the current month setpoint and read the keyboard
                    LDAB    MONTH           ; Get the current month setpoint for possible modification
                    SUBB    #11             ; Subtract 11 in case we will want to add 1
                    LSRA                    ; See if switch 1 was pressed
                    BCS     SET_DAY         ; If it was then leave the month and go on to the day
                    LSRA                    ; See if switch 2 was pressed
                    BCS     INC_MONTH_SET   ; If it was then increment the month
                    LSRA                    ; See if switch 3 was pressed
                    BCC     SET_MONTH       ; If none pressed, stay in this mode
                    ADDB    #10             ; If switch 3 was pressed then add back all but one of the     
                                            ;  11 that were subtracted above, to give a net decrement by 1
INC_MONTH_SET       TSTB                    ; After subtracting either 1 or 11, see if the 
                    BGT     SAVE_MONTH      ;  month is less than 1
                    ADDB    #12             ; If it is then add 12 to bring it back in the range 1 to 12
SAVE_MONTH          STAB    MONTH           ; Save the new month setpoint
                    BRA     SET_MONTH       ; Stay in this mode

;; PAGE 26 DISPLAY MODE ROUTINES

SET_DAY             EQU     *               ; Once the month is set, display and adjust the day
                    JSR     DAYS_IN_MONTH   ; First see that the day is valid for the current 
                    LDAA    DAY             ;  month setpoint [bill: days in month in register B]
                    LSRA                    ; Delete the daylight saving time flag
                    BEQ     SAVE_DAY        ; If the day is zero then set the day to last of month
                    SBA                     ; If it is greater than allowed then subtract the number of days
                    TAB                     ;  in this month
                    BGT     SAVE_DAY        ; Save the difference if it is greater than zero
                    LDD     #$03CB          ; Special select value to display the day and suppress
                                            ;  the month and indicator lights
                    JSR     BACKGROUND      ; Display the current day setpoint and read the keyboard
                    LDAB    DAY             ; Get the current day setpoint and read the keyboard
                    LSRB                    ; Delete the daylight saving flag
                    LSRA                    ; See if switch 1 was pressed
                    BCS     SET_HOUR        ; If it was then leave the day and go to the hour
                    LSRA                    ; See if switch 2 was pressed
                    BCS     INC_DAY_SET     ; If it was then increment the day
                    LSRA                    ; See if switch 3 was pressed
                    BCC     SET_DAY         ; If none pressed, stay in this mode
                    DECB                    ; If switch 3 was pressed then decrement the day
                                            ; If it decrements to zero then it will be changed to
                                            ;  the last day in the month above
                    DECB                    ; Decrement a second time to cancel the increment below
INC_DAY_SET         INCB                    ; Increment to the next day
                                            ; If it increments past the last valid day then
                                            ;  it will get set to day one above
SAVE_DAY            LSLB                    ; Shift in a zero for daylight saving flag
                    STAB    DAY
                    BRA     SET_DAY         ; Stay in this mode

SET_HOUR            EQU     *               ; Once the day is set, display and adjust the hour.
                    LDD     #$023B          ; Special select value to display the hour and suppress
                    JSR     BACKGROUND      ;  the minutes and indicator lights.
                                            ;  return after reading the keyboard
                    LDAB    TIME_OF_DAY     ; Get the current time for possible modification
                                            ;  [bill: represented as units of 10 minutes]
                    LSRA                    ; See if switch 1 was pressed
                    BCS     SET_TEN_MINUTE  ; If it was then leave the hour and go on to the
                                            ;  tens of minutes
                    LSRA                    ; See if switch 2 was pressed
                    BCS     INC_HOUR        ; If it was then increment the hour
                    LSRA                    ; See if switch 2 was pressed
                    BCS     SET_HOUR        ; If none pressed, stay in this mode
                    SUBB    #6              ; If switch 3 was pressed then decrement the hour
                    BHS     SAVE_HOUR       ; As long as this does not go below zero, save it
                    ADDB    #24*6           ; If it goes below zero then add 24 hours
                    BRA     SAVE_HOUR       

INC_HOUR            ADDB    #6              ; If switch 2 was pressed then increment the hour
                    CMPB    #24*6           ; Make sure this does not go above 24 hours
                    BLO     SAVE_HOUR       ; If it doesn't then save it
                    SUBB    #24*6           ; If it goes above 24 hours then subtract 24 hours
SAVE_HOUR           STAB    TIME_OF_DAY     ; Save the modified hour
                    JSR     INIT_MINMAX     ; Whenever then time of day is changed, reset the
;; PAGE 27
                                            ;  minimum, maximum, and average values
                    BRA     SET_HOUR        ; Stay in this mode

;; PAGE 28

SET_TEN_MINUTE      EQU     *               ; Once the hour is set, display and adjust the 
                                            ;  tens of minutes
                    LDD     #$02DB          ; Special select value to display the tens of minutes and
                    JSR     BACKGROUND      ;  suppress the hours and minutes and indicator lights.
                                            ;  return after reading the keyboard
                    LDAB    TIME_OF_DAY     ; Get the current time for possible modification
DIV_BY_6            SUBB    #6              ; Divide by six and save the remainder to show tens of minutes
                    BHS     DIV_BY_6        ; When done, this will be a value from -1 to -6
                                            ; [bill: this removes whole hours leaving just 10-minute units]
                    LSRA                    ; See if switch 1 was pressed
                    BCS     SET_MINUTE      ; If it was then leave the tens of minutes and go on
                                            ;  to the minutes
                    LSRA                    ; See if switch 2 was pressed
                    BCS     INC_TENS        ; If it was then increment the tens of minutes
                    LSRA                    ; See if switch 3 was pressed
                    BCC     SET_TEN_MINUTE  ; If none pressed, stay in this mode
                    LDAA    #-1             ; If switch 3 was pressed then decrement the tens of minutes
                    CMPB    #-6             ; Check the tens of minutes
                    BNE     ADJ_TENS        ; Unless the tens digit was zero, decrement it
                    LDAA    #5              ; If it was zero then add five to it
                    BRA     ADJ_TENS

INC_TENS            LDAA    #1              ; If switch 2 was pressed then increment the tens
                    INCB                    ;  check the tens of minutes
                    BNE     ADJ_TENS        ; Unless the tens digit was five, increment it
                    LDAA    #-5             ; If it was five then subtract five from it
ADJ_TENS            ADDA    TIME_OF_DAY     ; [bill: A has delta to full time; B had just 10-minutes]
                    STAA    TIME_OF_DAY     ; Put back the adjusted time
                    JSR     INIT_MINMAX     ; Every time the hour or minute is changed, reset
                                            ;  the minimum and maximum and mean values
                    BRA     SET_TEN_MINUTE  ; Stay in this mode
                    
;; PAGE 29 
SET_MINUTE          EQU     *               ; Once the tens of minutes are set, set the minutes
                    LDD     #$02EB          ; Special select value to display the minutes and suppress
                    BSR     BACKGROUND      ;  the hours and tens of minutes.
                                            ;  return after reading the keyboard
                    LSRA                    ; See if switch 1 has been pressed
                    BCS     SET_ALT         ; If it has then leave the minutes unchanged an go on
                                            ;  to modify the alternate display mode selection setpoint
                    BITA    #ACC0+ACC1      ; See if either switch 2 or switch 3 has been pressed
                    BEQ     SET_MINUTE      ; As long as neither of them has been pressed, let
                                            ;  the clock keep running
MOD_MINUTE          LDAB    TENTHS          ; Get the current minutes for possible change
                    LSRA                    ; See if switch 2 was pressed
                    BCS     INC_MINUTE      ; If it was then increment the minutes
                    LSRA                    ; See if switch 3 was pressed
                    BCC     HOLD_MINUTE     ; If not then just hold the time at its present value
                    ADDB    #$90            ; Decrement the upper digit of this BCD value
                    BRA     SKIP_INC        ; Skip the increment in the next line
INC_MINUTE          ADDB    #$10            ; Increment the upper digit of this BCD value
SKIP_INC            TBA                     ; Copy it into register A to decimal adjust the value
                    DAA
                    ANDA    #$F0            ; Set the tenths of miutes digit to zero

                    STAA    TENTHS          ; Save the modified minutes value

; original binary, instead of 'CLR TWENTIETHS':
; fbb6 86 78  HOLD_MINUTE  LDAA #$78
; fbb8 97 92               STAA $92 (TWENTIETHS)

HOLD_MINUTE         CLR     TWENTIETHS      ; Clear the lower bits of the time to keep
                                            ;  it set exactly to the minute - also keeps the
                                            ;  colon from flashing in the display
                    LDD     #$02EB          ; Special select value to display the minutes and suppress
                    BSR     BACKGROUND      ;  the hours and tens of minutes
                                            ;  return after reading the keyboard
                    LSRA                    ; See if switch 1 was pressed
                    BCC     MOD_MINUTE      ; If not they stay in this mode and hold the time
                                            ;  where it is
EXIT_SET            JMP     DISPLAY_MODE    ; On exiting from this mode, return
                                            ;  to normal display mode
;; PAGE 30

SET_ALT             EQU     *               ; If no change was made to the minutes then display
                                            ;  and adjust the alternate display mode selection setponit
                    LDAA    #$A0            ; Special select value to display the alternate mode
                    BSR     BACKGROUND      ;  selection setpoint.
                                            ;  return after reading keyboard
                    LDAB    ALT_SELECT      ; Get the current value of the setpoint just in case
                    LSRA                    ; See if switch 1 was pressed
                    BCS     EXIT_SET        ; If it was then leave this mode and return to normal
                                            ;  display mode
                    LSRA                    ; See if switch 2 was pressed
                    BCS     INC_ALT         ; If it was then go to the next select value
                    LSRA                    ; See if switch 3 was pressed
                    BCC     SET_ALT         ; If none pressed then stay in this mode
DEC_ALT             DECB                    ; If switch 3 was pressed then go to the previous value
                    BSR     VALID_ALT       ; See if this is a valid selection setpoint
                    BEQ     DEC_ALT         ; If not then go back another one
                    BRA     SAVE_ALT        ; When a valid one is found, save it

INC_ALT             INCB                    ; If switch 2 was pressed then go to the next value
                    BSR     VALID_ALT       ; See if this is a valud selection setpoint
                    BEQ     INC_ALT         ; If not then go forward another one
SAVE_ALT            STAB    ALT_SELECT      ; Once a valid one is found, save it
                    BRA     SET_ALT         ; Stay in this mode

VALID_ALT           ANDB    #$0F            ; Clear the upper for bits in case they get set
                    BEQ     INVALID_ALT     ; A value of zero is invalid, return equal
                    PSHB                    ; Save the lower 4 bits
TEST_ALT_BIT        LSRB                    ; Look for the first non-zero bit
                    BCC     TEST_ALT_BIT    ; Keep shifting until it is found
                    PULB                    ; At this point, if the rest of the byte is zero then this is
                                            ;  not a valid selection setpoint, so return equal
                                            ; If the rest of the byte is not zero then it is
                                            ;  valid, so return not equal
INVALID_ALT         RTS

;; PAGE 31

; Diagnostics routine

DIAG_MODE           EQU     *               ; This is a display routine in which various pieces of
                                            ;  information needed only for diagnostic purposes can
                                            ;  be accessed.  At present this consists of access to 
                                            ;  the contents of all the RAM locations.
                    LDAB    RAM_PTR         ; Load the address of the RAM location to display
                    ANDB    #$7F            ; Make sure it is pointing to one of the 128 RAM bytes
                    STAB    RAM_PTR         ; Save the pointer with the MSB clear
                    LDAA    #$90            ; Special select value to display ram address and contents
                                            ;  in hex, two digits each
                    CMPB    #$08            ; At address 0x88, display 8.8.:8.8. and all indicator lights
                    BNE     NOT_88          ;  turned on
                    LDD     #$C000          
NOT_88              BSR     BACKGROUND
                    LSRA                    ; See if switch 1 was pressed
                    BCS     EXIT_SET        ; If it was then exit from diagnostic mode
DIAG_UP_DOWN        LSRA                    ; See if switch 2 was pressed
                    BCS     INC_DIAG        ; If it was then increment the RAM address
                    LSRA                    ; See if swtch 3 was pressed
                    BCC     DIAG_MODE       ; If not then just continue displaying the present
                                            ;  RAM location
                    DEC     RAM_PTR         ; If switch 3 was pressed then decrement the pointer and
                    BRA     DIAG_MODE       ;  display the contents of the new location

INC_DIAG            INC     RAM_PTR         ; If switch 2 was pressed then increment the
                                            ;  pointer
                    BRA     DIAG_MODE       ; Display the contents of the new location

;; PAGE 32 BACKGROUND ROUTINE

; Main background routine
; At present this routine performs three functions: it formats
; information to be displayed, using a select code passed to it in
; the A and B registers; it waits for 1/20 second to elapse and
; completes a handshake with the foreground routine; and it reads
; the keyboard and decides which keys have just been pressed.  As
; long as no more than one key is pressed at a time, it the returns
; with this keyboard information to the same routine which had
; called it, but as soon as two keys are pressed simultaneiously it
; discards the return address and goes instead to one of three 
; special routines

BACKGROUND          EQU     *
                    TSTA                    ; First, see if the display mode is one of the special ones
                    BPL     NOT_SPECIAL     ; If the MSB is not set then standard display mode
                    LSLA                    ; IF the MSB is set then test the next bit as well
                    BPL     NOT_CONST       ; if this bit is set then one of the constant patterns
                                            ;  is to be displayed
                    LDD     #$FFFF          ; Presently the only constant pattern is the one with
                    STD     MSD             ;  all segments and lights on
                    STD     MSD+2
                    STD     MSD+4
                    JMP     END_DISP        ; Skip the digit blanking and copying from the temporary
                                            ;  buffer

;; PAGE 33
NOT_CONST           LSLA                    ; If the information to be displayed is not constant,
                    BPL     NOT_DISP_ALT    ;  see if it is alternate select mode
                    LDD     #C*$100+F       ; If it is then display the characters 'CFHd'
                    STD     TEMP_BUF
                    LDD     #H*$100+D
                    STD     TEMP_BUF+2
                    LDD     #SET+DEGC+DEGF+AM+PM+DATE+CURR ; Turn on all these indicator lights
                    STD     TEMP_BUF+4      ;  to show this mode
                    LDAB    ALT_SELECT      ; See which of these to blank
                    COMB                    ; Blank those whose select big was zero
                    LDAA    #$10            ; Shift it up to the upper four bits
                    MUL
                    JMP     BLANK_DISP      ; Blank and copy the digits

NOT_DISP_ALT        EQU     *               ; The only other special mode is RAM contents
                    LDAB    RAM_PTR         ; Format the address first
                    ORAB    #$80            ; Display its real value, with MSB set
                    PSHB                    ; Save the address for later
                    JSR     DISP_HEX        ; Format it as two digit hex
                    STD     MSD             ; Write it directly to the display buffer
                    PULB                    ; Get the address again
                    LDX     #0              ; This time use it to fetch the contents
                    ABX                     ; Form the complet address in X
                    LDAB    0,X             ; Load the contents
                    JSR     DISP_HEX        ; Format it
                    STD     MSD+2           ; Write it to the display buffer
                    LDD     #$FFFF-COLON-SET ; Turn on nearly all the indicator lights
                    STD     MSD+4           ;  in this mode
                    JMP     END_DISP        ; No need to blank digits or copy to display RAM

;; PAGE 34
NOT_SPECIAL         PSHB                    ; If none of the special display modes then save
                    LSRA                    ;  the zone selector and shift out the data type selector
                    ROR     TEMP_BUF        ; The LSB of the data type selector is saved for later
                    LSRA                    ;  as the second bit of the data type selector is shifted out
                    BCS     NOT_TEMP        ;  test it for temperature value vs. time value
                    JSR     POINTO_TEMP     ; If it is a temperature then get its address (in X)
                    LDAB    #DEGC           ; Assume that the units are degrees C
                    LDAA    TEMP_BUF        ; If this is wrong then the sign bit of temp_buf will
                    BPL     IS_DEGC         ;  be set
                    LDAB    #DEGF           ; In this case turn on the degree f light instead
IS_DEGC             STAB    TEMP_BUF+5      ;  write this value into the buffer
                    CPX     #TODAY_MEAN1 
                    BLO     NOT_AVG
                    CPX     #TODAY_MEAN3
                    BHI     NOT_AVG
                    JSR     AVERAGE
                    LDX     #TEMP+2
                    STD     0,X
NOT_AVG             JSR     CALC_TEMP       ; Convert to temperature in the desired units
                    JSR     DISP_FOUR       ; Now convert it to four digits on the stack
                    INC     TEMP_BUF+2      ; Turn on the decimal point to the right of the
                                            ;  third digit
                    PULB                    ; Get back the zone selection bits
                    JMP     BLANK_DISP      ; Blank digits as needed and copy to RAM

NOT_TEMP            TST     TEMP_BUF        ; If not temperature then see if it is time or date
                    BPL     DISP_TIME       ; If not date then must be time
                    TSTA                    ; If date then may be either month/day or year
                    BEQ     DISP_DATE       ; If no other bits set then it is month/day
                    LDD     #1984           ; The base year is 1984
                    ADDB    YEAR            ; Add this to the relative year value
                    ADCA    #0              ; Carry into the upper byte
                    JSR     DISP_FOUR       ; Display as a four digit value (should get no blanking)
                    LDD     #DATE           ; Select the indicator lights to use
                    STD     TEMP_BUF+4      ; Write them to the buffer
                    PULB                    ; Get back the zone selection bits
                    JMP     BLANK_DISP      ; Copy and blank as needed

DISP_DATE           LDAB    MONTH           ; For displaying the date start by formatting
                    JSR     DISP_TWO        ;  the month as two digits with leading zero blanking
                    STD     TEMP_BUF        ; Write this out to the first two digits
                    LDAB    DAY             ; Also format the day as two digits with blanking
                    LSRB                    ;  shift out the daylight saving flag
                    JSR     DISP_TWO
                    LDD     #DATE+CURR      ; Set up the indicator lights
                    STD     TEMP_BUF+4      ; Write them to the buffer
                    PULB                    ; Get back the zone selector value
                    JMP     BLANK_DISP      ; Blank digits if required and copy to RAM

;; PAGE 35
DISP_TIME           EQU     *
                    JSR     POINTO_TEMP     ; Get the address of the temperature associated with
                                            ;  this time
                    LDAA    2,X             ; Get the time value in A
                    CLRB                    ; Set the last digit of time to zero
                    CPX     #CONV_BUF+15    ; See if it is the current time which is needed
                    BHS     NOT_CUR_TIME    ; If it is then
                    LDAA    TIME_OF_DAY     ;  substitute the current time of day
                    LDAB    TENTHS          ;  and the current tenths of minutes, which has the
                                            ;  units digit of the minutes
NOT_CUR_TIME        LSRB                    ; Shift the minutes digit to the low four bits
                    LSRB
                    LSRB
                    LSRB
                    STD     TEMP            ; Save both bytes of the time value
                    BNE     NOT_2400        ; If both bytes are zero then it is 12:00 midnight
                    LDAB    PORT1           ; See if we are formatting 24 hour time
                    BITB    #JUMPER_24      
                    BEQ     NOT_2400        ; If it is 12:00 midnight and we are in 24 hour time
                    LDAA    #24*6           ;  then substitute the time 24:00
NOT_2400            LDAB    #171            ; If it is not midnight or we are in 12 hour time
                    MUL                     ;  mode then continue by multiplying by 1/6
                    LSRD                    ; 171 is actually 1024/6 so after we shift down two bits the
                    LSRD                    ;  whole number part is in A and the fraction is in B
                    STAB    TEMP            ; Save the fraction part
                    LDAB    PORT1           ; See if we are in 12 hour or 24 hour time mode
                    BITB    #JUMPER_24      
                    BEQ     TWELVE_HOUR     ; If we are in 24 hour mode then
                    SEC                     ;  set a bit to indicate it
                    ROL     TEMP+1
                    LDAB    #ZERO           ; In 24 hour mode don't suppress zeros
                    BRA     FORM_HOURS      ; /Format the hours digits exactly as they are

TWELVE_HOUR         CMPA    #12             ; In 12 hour mode see if it is AM or PM
                    BLO     IS_AM           ; If it is PM then
                    SUBA    #12             ;  shift it down to a value from 0 to 11
IS_AM               ROL     TEMP+1          ; Save the carry bit to show whether it is AM or PM
                    ROL     TEMP+1          ; Also save a zero bit to show that it is 12 hour time
                    TSTA                    ; See if it is zero hours
                    BNE     NOT_0           ; If it is then
                    LDAA    #12             ;   make it 12 hours
NOT_0               CLRB                    ; Assume initially that the tens of hours digit is zero
FORM_HOURS          SUBA    #10             ; See if it is at least one
                    BLO     GOT_TENS        ; If not then leave the tens digit blank
                    LDAB    #ONE            ; Otherwise assume that it is a one
                    SUBA    #10             ; See if is actually two
                    BLO     GOT_TENS        ; If not then leave it a one
                    LDAB    #TWO            ; Otherwise make it a two
                    SUBA    #10             ; Subtract another ten to match the results for zero and one
GOT_TENS            STAB    TEMP_BUF        ; Write the tens digit to the buffer
                    TAB                     ; Copy the negative residual to B
                    LDX     #DIGIT_TAB+10-256; Point to the lookup table but compensate for
                    ABX                     ;  the extra ten which was subtracted above
                    LDAB    0,X             ; Get the bit pattern for the second digit
                    STAB    TEMP_BUF+1      ; Write the second digit to the buffer
                    LDAB    TEMP            ; Get the fractional part from above to calcuate the

;; PAGE 36
                    LDAA    #6              ;  tens of minutes
                    MUL
                    TAB                     ; Copy this into B
                    LDX     DIGIT_TAB       ; Look up the bit pattern for this digit
                    ABX     
                    LDAB    0,X
                    STAB    TEMP_BUF+2      ; Write the third digit to the buffer
                    LDAB    TEMP+1          ; Get the four bits of the last digit
                    CLRA                    ; Assume that neither AM or PM will be turned on (24 hour mode)
                    LSRB                    ; Test the bit which was saved to show this
                    BCS     DISP_AM_PM      ; If it is set then leave both lights off
                    LDAA    #AM             ; If it is clear then assume that is is AM
                    LSRB                    ; The next bit will tell us which it is
                    BCS     DISP_AM_PM      ; If it is set then leave the AM light on
                    LDAA    #PM             ; If it is clear then turn on the PM light instead
DISP_AM_PM          STAA    TEMP_BUF+5      ; Save the second byte of indicator lights
                    LDX     #DIGIT_TAB      ; Index into the table
                    ABX
                    LDAA    0,X             ; Get the digit
                    LDAB    TEMP_BUF+4      ; Get the first byte of indicator lights
                    ORAB    #COLON/$100     ; Set the colon to show time display mode
                    STD     TEMP_BUF+3      ; Write out the fourth digit and the lights
                    PULB                    ; Get the zone selector value

; Drop directly into the display blanking routine

;; PAGE 37

; Copy the display data from the temporary buffer to the display
; buffer while blanking digits as required and blinking the colon

BLANK_DISP          PSHB                    ; Save the zone selector value
                    LDX     #0              ; Use X as a pointer and counter
BLANK_NEXT          LDAA    TEMP_BUF,X      ; Load a digit from the temporary buffer
                    LSLB                    ; See whether this one should be blanked
                    BCC     NOT_BLANK       ; If the bit is not set then don't blank the digit
                    CLRA                    ; Otherwise blank it
NOT_BLANK           STAA    MSD,X           ; Write the digit to the display buffer
                    INX                     ; point to the next digit
                    CPX     #4              ; See if all four digits have been copied
                    BLO     BLANK_NEXT      ; Repeat until all four done
                    LDAA    TEMP_BUF+4      ; Get the first byte of indicator light bits
                    LSLB                    ; See if the set light should be turned on
                    BCC     SET_OFF         ; If not then leave it as is
                    ORAA    #SET/$100       ; Otherwise turn it on
SET_OFF             LDAB    TWENTIETHS      ; See if the blinking colon should be on or off
SUB_20              SUBB    #20             ;  this time - get the present time mod 1 second.
                    BHS     SUB_20          ;  Keep subtracting 1 second until it goes negative
                    ADDB    #10             ; Add back 1/2 second
                    BMI     COLON_ON        ; If we are over 1/2 second into the current second then
                    ANDA    #$FF-(COLON/$100) ; Turn off the colon (if it was on)
COLON_ON            PULB                    ; Get back the original zone selector value
                    ANDB    #ACC0+ACC1      ; Use just the two lowest bits of the byte
                    LDX     #ZONE_TAB       ; Point to a lookup table for the zone indictor lights
                    ABX                     ; Index into it
                    LDAB    TEMP_BUF+5      ; Get the second byte of the indicator bits
                    ORAB    0,X             ; Or in the zone indicator bits
                    STD     MSD+4           ; Write both bytes to the display buffer

END_DISP            LDX     #-1             ; Set up a counter to measure processor loading
INC_LOAD            INX                     ; Increment it once every 9 microseconds
                    LDAA    ADC_STATE       ; Before continuing, wait for the foreground
                    BNE     INC_LOAD        ;  task to indicate that 1/20 second has elapsed
                    LDAA    #3              ; Reset the ADC state to 3 to complete the handshake with
                    STAA    ADC_STATE       ;  foreground
                    STX     TEMP_BUF+4      ; Put the loading value in a place where it will not
                                            ;  get in the way
                    LDAA    PORT1           ; Read the keyboard
                    TAB                     ; Make a second copy of the key closures
                    COMA                    ; Complement one copy so that a bit is set for each key that
                                            ;  is pressed
                    ANDA    LAST_KBD        ; And it with the previous reading of the keyboard,
                                            ;  uncomplemented, so that a bit will now be set only if
                                            ;  the key is pressed now but was not pressed 1/20 sec ago
                    STAB    LAST_KBD        ; Save the present key closures to use 1/20 sec from now
                    COMB                    ; Now also complement this to set the bit if the key is pressed
                    ANDA    #ACC0+ACC1+ACC2 ; Mask off any other bits in the returned byte
                    ANDB    #ACC0+ACC1+ACC2 ; Do the same for the current bits
                    PULX                    ; Take the return address of the stack in case we don't return
                    CMPB    #ACC0+ACC1      ; See if both switch 1 and switch 2 are being pressed
                    BNE     NOT_ALT         ;  simultaneiously
NOT_ALT             JMP     ENTER_ALT       ; If they are then go to alternate display mode

;; PAGE 38
                    CMPB    #ACC0+ACC2      ; See if both switch 1 and switch 3 are being pressed
                    BNE     NOT_SET         ;  simultaneously
                    JMP     SET_MODE        ; If they are then go to time setting mode
NOT_SET             CMPB    #ACC1+ACC2      ; See if both switch 2 and switch 3 are being pressed
                    BNE     NOT_DIAG        ;  simultaneously
                    JMP     DIAG_MODE       ; If they are then go to diagnostic mode
NOT_DIAG            JMP     0,X             ; If no pairs are being pressed then return to the calling
                                            ;  routine

ZONE_TAB            DC      ZONE1           ; Indicator light for zone 1
                    DC      ZONE2           ; Indicator light for zone 2
                    DC      ZONE3           ; Indicator light for zone 3
                    DC      0               ; Special value to suppress zone indicator lights

;; PAGE 39
; Subroutine to find the address of a temperature value from the
; type indicator in A and the zone number in B.
; Returns the address in X and the indicator light bits in
; TEMP_BUF+4

POINTO_TEMP         PSHB                    ; Set aside the zone number for now
                    TAB                     ; See what type of temp is desired
                    LDX     TEMP_TAB        ; Index into a table of temperature addresses and
                    ABX                     ;  indicator light bits. X + 3*B
                    LSLB
                    ABX                    
                    LDAA    2,X             ; Get the indicator light bits first
                    STAA    TEMP_BUF+4      ; Save them in RAM
                    LDX     0,X             ; Load the address from the table, whic his the address of
                                            ;  the temperature value for zone 1
                    PULB                    ; Get the zone number again
                    ANDB    #ACC0+ACC1      ; Look at just the zone selector bits
                    ABX                     ; Add three times the zone number to select the value for the
                    LSLB                    ;  specific zone
                    ABX
                    RTS                     ; Return

; Subroutine to calculate the temperature in tenths of a degree,
; either C or F, from the A/D values
; Enter with the address of the A/D value in X and a flag to select
; degrees C or F in the sign bit of TEMP_BUF
; Returns the temperature in A and B

CALC_TEMP           EQU     *
                    LDD     CONV_BUF+12     ; Calculate the difference between the high reference
                    SUBD    CONV_BUF+9      ;  and the low reference - this is 100C or 180F
                    LSLD                    ; We will be dividing twice this value into half of the other
                                            ;  value so the useable temperature range is 400c or 720f
                    STD     TEMP            ; Set this aside as our divisor
                    LSRD                    ; Now calculate the difference between the high reference and
                    ADDD    CONV_BUF+12     ;  the temperature conversion value, but offset it
                                            ;  upward by 100c or 180f to make sure it is positive
                    SUBD    0,X             ; We will sutract the offset out later
                    LSRD                    ; Divide by two to give sufficient range
                    LDX     #16             ; Initialize a counter to generate a 16-bit quotient
TEMP_LOOP           SUBD    TEMP            ; Perform a trial subtraction of the divisor
                    BHS     IS_POSITIVE     ; If the remainder is still positive then leave it
                    ADDD    TEMP            ; Otherwise add the divisor back in
IS_POSITIVE         ROL     TEMP+3          ; Shift the next bit of the quotient from the carry bit
                    ROL     TEMP+2          ;  and shift the entire quotient up one bit
                    LSLD                    ; Double the remainder
                    DEX                     ; Decrement the counter
                    BNE     TEMP_LOOP       ; Repeat for 16 bits of quotient
                    LDAB    TEMP+3          ; Get the low byte of the quotient
                    COMB                    ; Correct for the bit inversion in the above divide
                    LDAA    TEMP_BUF        ; See if we want degrees C or F
                    BPL     CALC_C          ; If it is C then use a different version of the scaling
                    LDAA    #180*10/8       ; For degrees F the scaling factor is 1800 tenths of 
                    MUL                     ;  a degree - any powers of two will be removed by shifting
                    
;; PAGE 40
                    ADDA    #2              ; Round up 1/20 degree
                    STAA    TEMP            ; This is part of the second byte of the final result
                    LDAA    TEMP+2          ; Get the high byte of the quotient
                    COMA                    ; Again correct for the bit inversion
                    LDAB    #180*10/8       ; Scale this in the same way
                    MUL
                    ADDB    TEMP            ; Combine the two parts of the lower byte of the result
                    ADCA    #0              ; Carry into the upper byte
                    LSRD                    ; Shift down two bits to get the correct scaling
                    LSRD
                    SUBD    #(180-32)*10    ; This cancels the extra 180 degrees which were
                                            ;  added in to make the values positive and corrects to 32 degrees
                                            ;  as the freezing point of water in Fahrenheit
                    RTS

CALC_C              LDAA    #100*10/8       ; For degrees C the scaling factor is 1000 tenths
                    MUL                     ;  of a degree
                    ADDA    #2              ; Round up 1/20 degree
                    STAA    TEMP            ; Save part of the low byte of the result
                    LDAA    TEMP+2          ; Get the high byte of the quotient
                    COMA                    ; Correct for bit inversion
                    LDAB    #100*10/8       ; Scale it
                    MUL
                    ADDB    TEMP            ; Finish calculating the low byte
                    ADCA    #0              ; And the high byte
                    LSRD                    ; Shift down two bits to get final scaling
                    LSRD
                    SUBD    #100*10         ; Cancel the extra 100 degrees
                    RTS                     ; Return the value

;; PAGE 41

; Lookup tables of temperature value addresses and indicator bit
; values

TEMP_TAB            DC.W    CONV_BUF        ; Current temperature
                    DC.B    CURR/$100
                    DC.W    TODAY_MAX1      ; Today's high
                    DC.B    (TODAY+HIGH)/$100
                    DC.W    TODAY_MIN1      ; Today's low
                    DC.B    (TODAY+LOW)/$100
                    DC.W    TODAY_MEAN1     ; Today's mean
                    DC.B    (TODAY+MEAN)/$100
                    DC.W    LAST_MAX1       ; Yesterday's high
                    DC.B    (YEST+HIGH)/$100
                    DC.W    LAST_MIN1       ; Yesterday's low
                    DC.B    (YEST+LOW)/$100
                    DC.W    LAST_MEAN1      ; Yesterday's mean
                    DC.B    (YEST+MEAN)/$100

;; PAGE 42

; Formatting routines

; What follows is a variety of formatting routines for various
; combinations of input and output information.  The output information
; is always a compbination of displayable digits returned in 
; TEMP_BUF through TEMP_BUF+3

; Subroutine to display a two digit value from 0 to 99, with zero 
; suppression on the first digit

DISP_TWO            EQU     *               ; The value to be formatted is passed in B
                    LDAA    #205            ; The first objective is to multiple by 1/10 and separate
                    MUL                     ;  the whole number and fraction parts of the result
                    LSRD                    ; Since the value 205 is about eight times 256/10, shift the
                    LSRD                    ;  answer down 3 bits and whole number will be in A and
                    LSRD                    ;  the fraction in B
                    PSHB                    ; Save the fraction temporarily on the stack
                    TAB                     ; Copy the whole number into B
                    BEQ     SUPP_TWO        ; If the whole number part is zero, supress it
END_TWO             LDX     #DIGIT_TAB      ; If it is not zero then index into a lookup
                    ABX                     ;  table to get the bits for this digit
                    LDAA    0,X
SUPP_TWO            PULB                    ; Get the fraction back off the stack
                    STAA    TEMP_BUF+2      ; Save the first digit in the buffer
                    LDAA    #10             ; Now multiply the fraction by ten to get the second digit
                    MUL
                    ADCA    #0              ; Round up to prevent truncation errors
                    TAB                     ; Copy the digit into B
                    LDX     #DIGIT_TAB      ; Also index into the table
                    ABX                     ; Do not do zero supression since this is the last digit
                    LDAB    0,X             ; Get the segments bits
                    STAB    TEMP_BUF+3      ; Save the second digit in the buffer
                    LDAA    TEMP_BUF+2      ; Return both digita in registers as well in case
                    RTS                     ;  the calling routine wants to write them into another location

;; PAGE 43

; Subroutine to display a four digit vlaue from -999 to 2999, with
; zero suppression on the first two digits

DISP_FOUR           EQU     *               ; The value to be formatted is pass in A and B
                    STD     TEMP            ; Save the unmodified vlue to be formatted
                    BPL     PLUS_FOUR       ; Test the sign at the same time
                    LDAA    #SEGG           ; If the value is negative, then maket he first digit '-'
                    STAA    TEMP_BUF        ; Write this character into the first digit in the buffer
                    CLRA                    ; Now negate the negative value to give a positive number
                    CLRB                    ;  to be formatted
                    SUBD    TEMP            
                    BRA     CONTIN_FOUR     ; Skip ahead to the second digit formatting
PLUS_FOUR           SUBD    #1000           ; If the value is positive, see if it is more
                    BHS     ONE_THOUSAND    ;  than 999
                    CLRA                    ; If not then make the first digit blank
                    BRA     SAVE_FIRST      ; Push it onto the stack and continue from there

ONE_THOUSAND        STD     TEMP            ; If the value is more than 999 then save the
                                            ;  value less than one thousand
                    SUBD    #1000           ; See if the remainder is still more then 999
                    BHS     TWO_THOUSAND    ; If it is then the first digit is assumed to be 2
                    LDAA    #ONE            ; But if not then make the first digit 1
                    BRA     SAVE_FIRST      ; Push it on the stack and continue from there

TWO_THOUSAND        STD     TEMP            ; If the value is more than 1999 the save
                                            ;  the value less two thousand
                    LDAA    #TWO            ; Make the first digit 2
SAVE_FIRST          STAA    TEMP_BUF        ; Save it in the buffer
                    LDD     TEMP            ; Get the value mod 1000
CONTIN_FOUR         EQU     *
                    LDX     #DIGIT_TAB-1    ; Point to the table of digit patterns
                    INX                     ; Keep incrementing into this table and subtracting 100 from
SUB_100             SUBD    #100            ;  the value until it goes negative
                    BHS     SUB_100         ; As soon as it goes negative we are pointing to the digit
                                            ;  to be displayed
                    ADDB    #100            ; Cancel the extra subtraction
                    LDAA    0,X             ; Load the second digit from the lookup table
                    CMPA    #ZERO           ; See if the second digit is a zero
                    BNE     SEC_NOT_ZERO    ; If second digit is zero then should it be blanked
                    LDAA    TEMP_BUF        ; Look at the first digit
                    BEQ     BLANK_SECOND    ; If first digit is blank then blank second
                    SUBA    #SEGG           ; Also, if the first digit is '-' then blank second
                    BEQ     BLANK_SECOND
                    LDAA    #ZERO           ; Otherwise put an actual zero that digit
BLANK_SECOND        EQU     *               ; Either a zero or a blank will be pushed next
SEC_NOT_ZERO        STAA    TEMP_BUF+1      ; Save the second digit, whatever it is
                    LDAA    #205            ; Duplicate the first part of the routine above for
                    MUL                     ;  formatting a two digit number, so that we can enter that
                    LSRD                    ;  routine and format the last two digita without allowing
                    LSRD                    ;  the first digit to be blanked
                    LSRD
                    PSHB
                    TAB
                    BRA     END_TWO         ; Now jump into the two digit formatter

;; PAGE 44

; Subroutine to display a two digit hex value
DISP_HEX            TBA                     ; Make a duplicate copy of the value to be displayed
                    LSRB                    ; Isolate the top four bits of the copy in B
                    LSRB
                    LSRB    
                    LSRB
                    LDX     #DIGIT_TAB      ; Point to the lookup table of both decimal and hex
                    ABX                     ;  bit patterns and index into it
                    TAB                     ; Copy the original value back into B
                    LDAA    0,X             ; Get the pattern for the first digit in A
                    ANDB    #$0F            ; Isolate the bottom four bits this time
                    LDX     #DIGIT_TAB      ; Again index into the table
                    ABX     
                    LDAB    0,X             ; Now we have the bit apttern for the second digit
                    RTS                     ; Return with both digit patterns in registers

;; PAGE 45

; Lookup table of bit patterns for each digit
DIGIT_TAB           DC      ZERO, ONE, TWO, THREE, FOUR
                    DC      FIVE, SIX, SEVEN, EIGHT, NINE
                    DC      A,B,C,D,E,F

;; PAGE 46

; Restart and initialization

; When power is first turned on and the processor starts running 
; there are cetain things which must be done before the instrument
; can be allowed to operate normally.  This incluides setting up the
; hardware to the desired mode of operation, initializing the
; contents of memory to a konwn state, performing certain tests on
; the hardware, allowing the A/D circuitry to settle and putting
; meaningful values in for the minimum, maximum, and mean
; temperatures

RESTART             EQU     *               ; The pointer in the vector table causes executeion to
                                            ;  begin at this point after a reset
                    CLRA                    ; Begin by setting up the internal hardware
                    STAA    $00             ; Make all 8 bits of port1 into inputs
                    COMA   
                    STAA    $04             ; Make all 16 bits of ports 3 and 4 into outputs
                    STAA    $05
                    STAA    SER_MODE_REG    ; Set the serial port up for external clock
                    LDAA    #$12            ; Make some of the bits in port 2 inputs and some outputs
                    STAA    $01             ; Bits 0, 2, and 3 are inputs, bits 1 and 4 are outputs
                    LDAA    #TIMER_EOCI     ; Set the bit to allow output compare interrupts
                    STAA    TIMER_CONTROL

                    LDX     #$80            ; Initialize the contents of all 128 bytes of internal RAM
CLEAR_RAM           CLR     $7F,X           ;  to zero
                    DEX
                    BNE     CLEAR_RAM
                    LDAA    #1              ; Set the month to 1
                    STAA    MONTH
                    STAA    A_D_SELECT      ; Also set the A/D converter to select channel 1
                    INCA                    ; Set the day to 2
                    STAA    DAY
                    LDAA    #$0F            ; Set the alternate display mode selection value to
                    STAA    ALT_SELECT      ;  select all four display modes
                    LDD     #$FFFF          ; Finally, initialize the display memory to turn on all
                    STD     MSD             ;  eight segments of all six digits for the first few
                    STD     MSD+2           ; seconds as a means of testing all of that hardware
                    STD     MSD+4

                    LDS     #$FF            ; Initialize the stack pointer to point at the very last
                                            ;  RAM location - the stack will occupy the section
                                            ;  from here down to hopefully just short of the 
                                            ;  last RAM location used for other purposes

                    CLI                     ; Here we Go!!!
                                            ; Since the only interrupt enabled in the system is the
                                            ;  output compare interrupt, as soon as the main interrupt
                                            ;  enable bit is cleared (it is actually an interrupt 
                                            ;  inhibit) we can begin to get output compare interrupts

;; PAGE 47

; Wait several seconds while things settle down
                    LDD     #$300+40        ; Load register A with a 3 and register B with a 40
                                            ; The 3 will be used to communicate with the foreground
                                            ;  task and the 40 will allow us to delay for two
                                            ;  seconds (40 twentieths)
WAIT_ZERO           TST     ADC_STATE       ; Each time a twentieth of a second has elapsed, this
                    BNE     WAIT_ZERO       ;  state counter will be decremented to zero - until then
                                            ;  just remain in this loop waiting
                    STAA    ADC_STATE       ; The background task must now complete a handshake
                                            ;  with the foreground task to let it know that it has
                                            ;  seen the indication of the completion of a twentieth
                                            ;  of a second and is ready to go on the the next twentieth
                    DECB                    ; After the end of each twentieth, decrement the counter in
                    BNE     WAIT_ZERO       ;  register B to see if two seconds have passed yet

;; PAGE 48
; Now that the adc has been running for a few seconds, all of the 
; voltage levels should have settled down and the individual
; measurements should all be valid, but since the value which is saved
; internally is a running average of a large number of 
; measurements it could take a long time before this filtered value
; is reasonably close to the actual answer. To avoid this, it is
; necessary that we initialize the five filtered values to something
; appropriate. The way we do that is to reset the filtered value
; to zero, take one more measurement on each of the five channels,
; which will result in a filtered value which is one fourth 
; of this single reading, then multiply this value by four 
; and put it back

                    LDX     #15             ; The conversion buffer is 15 bytes long
CLEAR_BUF           CLR     CONV_BUF-1,X    ; clear the buffer starting with the last byte
                    DEX                     ; Decrement the combined counter and pointer
                    BNE     CLEAR_BUF       ; Repeat until all 15 bytes are zero

; Note that because of the synchronization we achieved above with
; the foreground task there is no danger of not finishing the
; clearing of the buffer before the completion of the next
; measurement.

                    LDAB    #5              ; Set up register B as a counter to count out five more
                                            ;  conversions - register A still contains a three
LOOP_TST            TST     ADC_STATE       ; Once again, wait for foreground to complete a
                    BNE     LOOP_TST        ;  twentieth of a second
                    STAA    ADC_STATE       ; Complete the handshake
                    DECB                    ; Do this five times
                    BNE     LOOP_TST

                    LDX     #15             ; Use X as a combined counter and pointer while multiplying
                                            ;  each of the values in the buffer by 4
INIT_CONV_BUF       LDD     CONV_BUF-3,X    ; Get a value from the buffer
                    LSLD                    ; Shift it left two bits
                    LSLD
                    STD     CONV_BUF-3,X    ; The last two bits will all be zero, but their
                                            ;  significance is very small
                    DEX                     ; Back up to the previous location in the buffer
                    DEX
                    DEX
                    BNE     INIT_CONV_BUF   ; Repeat until all five filtered values have been
                                            ;  initialized
