# ThreeN1 -  Collatz conjecture computation

The Collatz conjecture is one of the most famous unsolved problems in mathematics. 
It concerns sequences of integers in which each term is obtained from the previous term as follows: 
- if a term is even, the next term is one half of it. 
- If a term is odd, the next term is 3 times the previous term plus 1. 

The conjecture is that these sequences always reach 1, no matter which positive integer is chosen to start the sequence. 
No general proof has been found at the moment.

ThreeN1 is a command line application that tries to prove this conjecture for any specific range of integer numbers.
It applied Collats conjecture rules to each number till it reaches 1.
Additionally it collects some statistics for each integer: number of steps to reach 1 and maximum number reached during calculation.

#How to use TheeN1

Call ThreeN1.exe from command line and specify arguments that define range of numbers to check and other parameters that take effect on calculation time,
 e.g. number of threads, whether to use cache, and so on.
 
 Usage: ThreeN1.exe -command <arg1>...<argN>  -option <arg1> <arg2>
 
 Examples:
 ThreeN1.exe -r 100M 200M
 ThreeN1.exe -s 100G 100M -c
 ThreeN1.exe -n 123456789
 ThreeN1.exe -l -s 100Z 500M
 
 ```
-r, --range     <start> <end> Define calculation range by specifying start and end values of the range
-s, --size      <start> <len> Define the calculation range by specifying start and length of the range
-n, --number    <number>      Calculate one number and show full chain of 3n1 numbers till 1 reached
-t, --threads   <threads>     Use specified number of threads for calculations (faster)
-c, --cache                   Use cache during calculations (cache is loaded from a file)
-f, --cachefile <filename>    Specify cache file name. Valid only with options -c and -g. Otherwise ignored
-l, --long                    Force using long arithmetic. If -l is specified long arithmetic will be used even for small numbers
                              By default 64bit integer arithmetic is used for numbers less than MAX_UINT64/3 
-g, --gen                     Fill cache during calculation and save it into file. Valid only when option -c is specified
                              Cache range is not equal to the range specified by -r, but based on it.
-h, --help                    Show this help
 ```
 
 #Specify range for calculations
 
 Range can be specified by two ways: using -r option and using -s option.
 -r option requires range start and range end values
 -s option requires range start and range length values
 
 Advantage of -s option is that you can easy specify huge value from range start and small value for range length.
 For example `-s 100P 100M`.
 For -r option this range would look like this - `-r 100P 100'000'000'100M`.
 
 To specify big values for range options you can use suffixes "B", "K", "M", "G", "T", "P", "E", "Z", "Y".
 "K" means that value will be multiplied by 1 000
 "M" means that value will be multiplied by 1 000 000
 and so on 
 "Y" means that value will be multiplied by 10^24.
 
 Also it is possible to use symbol ''' (single quote) as thousands separator in arguments for -r and -s command line options.
 Examples:
  ThreeN1.exe -s 1'000'000K 10'000
  ThreeN1.exe -r 100G 100'001M
 
 #Calculate single number.
 
 You can calculate single number and see what values this number reaches and how many steps it requires to get to 1.
 Use command line option -n for that.
 Examples:
 ThreeN1.exe -n 100'000K
 ThreeN1.exe -n 100'000'123'456'789
 
 The output will be as following:
 
 ```
Collatz conjecture solver (3n+1)

Calculating chain for single number using Collatz rules.

Starting number            : 123'456'789
Chain of numbers           : 123'456'789,370'370'368,185'185'184,92'592'592,46'296'296,23'148'148,11'574'074,5'787'037,17'361'112,8'680'556,4'340'278,2'170'139,6'510'418,3'255'209,9'765'628,4'882'814,2'441'407,7'324'222,3'662'111,10'986'334,5'493'167,16'479'502,8'239'751,24'719'254,12'359'627,37'078'882,18'539'441,55'618'324,27'809'162,13'904'581,41'713'744,20'856'872,10'428'436,5'214'218,2'607'109,7'821'328,3'910'664,1'955'332,977'666,488'833,1'466'500,733'250,366'625,1'099'876,549'938,274'969,824'908,412'454,206'227,618'682,309'341,928'024,464'012,232'006,116'003,348'010,174'005,522'016,261'008,130'504,65'252,32'626,16'313,48'940,24'470,12'235,36'706,18'353,55'060,27'530,13'765,41'296,20'648,10'324,5'162,2'581,7'744,3'872,1'936,968,484,242,121,364,182,91,274,137,412,206,103,310,155,466,233,700,350,175,526,263,790,395,1'186,593,1'780,890,445,1'336,668,334,167,502,251,754,377,1'132,566,283,850,425,1'276,638,319,958,479,1'438,719,2'158,1'079,3'238,1'619,4'858,2'429,7'288,3'644,1'822,911,2'734,1'367,4'102,2'051,6'154,3'077,9'232,4'616,2'308,1'154,577,1'732,866,433,1'300,650,325,976,488,244,122,61,184,92,46,23,70,35,106,53,160,80,40,20,10,5,16,8,4,2,1
Steps                      : 177
Max number in chain        : 370'370'368
Total time spent           : 0 seconds 1 ms
 ```
#Use cache during calculation

Using cache speeds up the calculation.

Cache file has name "ThreeN1 cache - 300M-600M.diffvar".
It is used by default. If you would like to use another file (for example generated by you using option -g) you can specify your own cache file.

ThreeN1.exe -s 1G 100M -c -f mycachefile.cache

#Multiple threads
You can tell ThreeN1 to use multiple threads for doing calculation.
 
ThreeN1.exe -t 5 -s 1T 1'000M - 5 threads will be used for doing calculations.
In this case ThreeN1 will divided specified into several subranges. Each thread will take its own subrage and do calculation independently from others. 
This way different threads do not compete for resources.
Please note that ThreeN1 output to console is different when multiple threads are requested.

#Short and long arithmetic

If possible ThreeN1 tries to use short arithmetic for its calculations.
Short arithmetic is 64bit arithmetic supported by popular processors.
It is extremely fast, but limited by ~18*10^18 integer number.
When specified range fits into 64bit integer range then ThreeN1 uses short arithmetic. 
If range is out of 64bit integers them ThreeN1 automatically switches to long arithmetic.
Long arithmetic does not have any limits and can calculate any number. The only disadvantage it is much slower than 64bit arithmetic.

With option -l you can forcibly turn on long arithmetic for small numbers. For example to check how it works at all.




 

 