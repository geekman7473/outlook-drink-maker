import serial
import time

arduino = serial.Serial(port='COM5', baudrate=9600, timeout=.1)

# numbers for drinks in liquor holders
vodka = 1
gin = 2
tequila = 4
white_rum = 5
triple_sec = 7
simple_syrup = 8
lime_juice = 9
cranberry = 11
orange_juice = 10

#pump amounts in fl.oz
recipes = {
    "Lemon Drop" : [(vodka, 2), (triple_sec, 1), (lemon_juice, 1), (simple_syrup, 1)],
    "DarkNStormy" : [(dark_rum, 2), (ginger_ale, 4), (lime, 1)],
    "Tom Collins" : [(gin, 2), (club_soda, 4), (simple_syrup, 1), (lemon_juice, 1)],
    "Moscow Mule" : [(vodka, 2), (ginger_ale, 4), (lime, 1)],
    "Cosmo" : [(vodka, 2), (cranberry, 4), (lime, 1), (triple_sec, 1)],
    "Tequila Sunrise" : [(tequila, 2), (grenadine, 1), (orange_juice, 4)],
    "Daiquiri" : [(white_rum, 2), (lime, 1), (simple_syrup, 1)],
    "Margarita" : [(tequila, 2), (triple_sec, 1), (lime, 1), (simple_syrup, 1)],
    "Long Island Iced Tea" : [(vodka, 1), (dark_rum, 1), (tequila, 1), (gin, 1), (triple_sec, 1), (coke, 1)],
    "Rum N Coke" : [(dark_rum, 2), (coke, 6)],
    "Cuba Libre" : [(dark_rum, 2), (coke, 5), (lime, 1)],
    "Gin Fizz" : [(gin, 2), (lemon_juice, 1), (club_soda, 4), (simple_syrup, 1)],
    "Whiskey Sour" : [(whiskey, 2), (lemon_juice, 1), (simple_syrup, 1)],
    "White Lady": [(gin, 2), (triple_sec, 1), (lemon_juice, 1)],
    "Vodka Cran": [(vodka, 2), (cranberry, 5)],
    "Screwdriver": [(vodka, 2), (orange_juice, 5)],
    "Ginger Highball" : [(whiskey, 2), (lemon_juice, 1), (simple_syrup, 1), (ginger_ale, 4)]
}

def write(x):
    arduino.write(bytes(x, 'utf-8'))
    time.sleep(1)

def make_drink(recipe_name):
    recipe = recipes[recipe_name]
    for item in recipe:
        while "$ DONE" not in str(arduino.readline()):
            pass
        arduino.write("MOVETO {}".format(item[0]))
        while "$ DONE" not in str(arduino.readline()):
            pass
        for i in range(item[1]):
            arduino.write("DISPENSE {}".format(item[0]))
            while "$ DONE" not in str(arduino.readline()):
                pass

    arduino.write("HOME")

while True:
    # First we wait for the machine to be in a ready state
    print("...")

    num = input("Enter a command: ") # Taking input from user
