# OS_project1

## Client-Server Application
This repository contains two codes for the client and server applications used for communication and data exchange between the client and server. The client application can ask questions to the server, and the server can respond to the client.

## Data Structures
### Client
#### name: The username of the client
#### lastAnswer: The last answer entered by the user
#### page: The current page number of the user
#### role: The user role (1 for student and 2 for teaching assistant)
#### last_q_index: The last question number asked by the user
#### socket: The client socket number

### Student
#### userPass: The username and password of the student

### Teaching Assistant
##### userPass: The username and password of the teaching assistant

### Question
#### q: The question text
#### status: The status of the question (true or false)
#### port: The port associated with the question
#### cid: The client ID connected to the question

## Main Functions
#### setupServer: Set up the server and establish a connection on the specified port
#### acceptClient: Accept a new client connection
#### login: Function for user login
#### page_handler: Function for managing user pages
#### write_to_file: Function for writing the question and answer to a file
#### get_question_number: Function for retrieving the question number from the file

## Server Setup
To set up the server, first set the desired port in the port variable in the main function. Then run the program. The server will run on the specified port and wait for client connections.
