# Miniproject_Module1
Project Module 1 C Embedded Programming - Sensor network simulation

This program aims to provide an as sufficient as possible system of governing multiple sensors. Because of our lack of knowledge and real life application, we can only simulate 1 data type/sensor (which means a sensor only measures 1 type of environmental value). 

-------------------------------------------
# INPUT 
Since it is only simulation, we limit the input to a strictly formatted file of input sensors that you should adhere to, presented as follows: You should
- Create a file called SENSOR_FILE.txt
- Format the sensor EACH LINE as: Index_number(or STT) Sensor_name Sensor_type Frequency(in Hz) Max_value
- Notes:
  + Each factor is separated with a SPACE
  + Sensors' types are limited only to these following: "TEMP" / "HUMID" / "GAS" / "LIGHT"
  + Sensor's name must be written as a whole (exp: TEMP_1, Sensor_1,...)
  + Max_value is the required value for the actuator (connected to that sensor) to activate.

- Input data: Since there is no real sensor to use, we can only simulate the input data to the sensor by scanf() in the terminal with float datatype.
 -------------------------------------------
# OUTPUT
The system will proceed the input data, check its validity and whether it exceeds the maximum value then an average filter will be applied to smoothen out the signal (avoiding unwanted interruptions of sudden high value data), then upload the data to a sensor_file_data.txt created as each sensor is initialized. The program will run in a loop until the user type ctrl + C to end the program, then it will return a REPORT FILE and a ERROR FILE containing several requirements:
- Number of invalid data
- Number of overload data
- Sensor with the most error
- Max/Min/Avg value for each sensor 
- Number of overflow

--------------------------------------------
# OTHER FEATURES
As you can see in the library file, there exists 2 functions covering the action of adding and deleting a Node (or a sensor). This is an additional feature that we havent tested yet but the logic is worth it.
