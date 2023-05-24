import mysql.connector

mydb = mysql.connector.connect(
  host="localhost",
  user="bf",
  password="root"
)

print(mydb)