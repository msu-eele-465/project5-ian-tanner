# Project 5 Workload Distribution Form: 2 Partners

It is recommended that at least some of the functionality is a shared responsibility between two or more members of the team, whereas some of the functionality is assigned solely to an individual. That said, how you break up the project is entirely up to you, so long as the workload distribution (measured in points as defined below) is approximately balanced.

You are encouraged to support each other with an overall goal of successfully completing the project. If one person struggles with their responsibility, the other member is encouraged to step in. However, an agreement should be struck between the partners wherein one partner gives up some of the assigned points for that function relative to the amount of help received.  

You will fill out this form:
1. Before the project starts. This is to document your intention. 
2. Before the demo if there was any deviation from the initial plan.

| Requirement/specification                                                                                 | Points | Partner 1 | Partner 2 |
|-----------------------------------------------------------------------------------------------------------|--------|-----------|-----------|
| **Presentation**                                                                                          |        |           |           |
| Introduction                                                                                              | 1      |     1     |           |
| Circuit diagram                                                                                           | 1      |     1     |           |
| System architecture diagram                                                                               | 1      |     1     |           |
| High-level flowchart                                                                                      | 1      |     1     |           |
| Professional demo                                                                                         | 2      |     1     |     1     |
| **Project Requirements**                                                                                  |        |           |           |
| The system must sample an LM19 temperature sensor.                                                        | 2      |     2     |           |
| Sample the temperature sensor every 0.5 s.                                                                | 0.5    |     .5    |           |
| The system must calculate a moving average of n temperature values.                                       | 1      |     1     |           |
| The default window size is n = 3.                                                                         | 0.5    |     .5    |           |
| The window size will be displayed in the bottom-right corner of the LCD.                                  | 0.5    |           |     .5    |
| In the "window size entry" state, the LCD will display "set window size" on the first line of the LCD:    | 0.5    |           |     .5    |
| When unlocked, the user can select a button on the keypad to enter the "window size entry" state.         | 0.5    |           |     .5    |
| The user can enter a single-digit number greater than 0 on the keypad to set the window size.             | 1      |           |     1     |
| After the user has entered the window size, the system should exit the "window size entry" state.         | 0.5    |           |     .5    |
| In the "pattern number entry" state, the LCD will display "set pattern" on the first line of the LCD:     | 0.5    |           |     .5    |
| When unlocked, the user can select a button on the keypad to enter the "pattern number entry" state.      | 0.5    |           |     .5    |
| In the "pattern number entry" state, the user can enter a number on the keypad to select the LED pattern. | 1      |           |     1     |
| After the user has entered the pattern number, the system should exit the "pattern number entry" state.   | 0.5    |           |     .5    |
| The temperature will be displayed in the bottom-left corner of the LCD.                                   | 0.5    |           |     .5    |
| The temperature will be displayed in degrees Celsius.                                                     | 2      |           |     2     |
| The temperature should not be displayed until n samples have been collected.                              | 0.5    |    .5     |           |
| The LED pattern name is displayed on the first line of the LCD:                                           | 0.5    |           |    .5     |
| After n samples have been collected, the new average temperature is displayed after every new sample.     | 0.5    |    .5     |           |
| The pattern name is only displayed when the system is unlocked and not in one of the "user entry" states. | 0.5    |           |     .5    |
| Total                                                                                                     | 20     |    10     |     10    |


## 🚀 Extra credit points
The extra credit points are allotted to both partners. You don't need to formally split these tasks. The points are just here for your information.

| Extra credit requirement/specification                                                                   | Extra Credit Points |
|----------------------------------------------------------------------------------------------------------|---------------------|
| The user can enter a multi-digit number for the window size                                              | 1                   |
| The user can switch between displaying the temperature in Celsius and Fahrenheit                         | 1                   |
