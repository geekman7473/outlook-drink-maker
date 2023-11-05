import flask
import threading
import atexit
import serial
import time
from flask import jsonify
from flask import request
from flask import redirect
from flask import url_for
from flask import render_template
import asyncio
import json

arduino = serial.Serial(port='COM6', baudrate=115200, timeout=.1)

# numbers for drinks in liquor holders
vodka = 1
gin = 2
whiskey = 3
tequila = 4
white_rum = 5
dark_rum = 6
triple_sec = 7
simple_syrup = 8
lemon_juice = 9
lime_juice = 10


#numbers for mixers in pumps
coke = 101
ginger_ale = 102
#orange_juice = 101
club_soda = 106
cranberry = 104


recipes = {
    "Lemon Drop": [(vodka, 2), (triple_sec, 1), (lemon_juice, 1), (simple_syrup, 1)],
    "Dark & Stormy": [(dark_rum, 2), (ginger_ale, 100), (lime_juice, 1)],
    "Tom Collins": [(gin, 2), (club_soda, 100), (simple_syrup, 1), (lemon_juice, 1)],
    "Moscow Mule": [(vodka, 2), (ginger_ale, 100), (lime_juice, 1)],
    "Cosmo": [(vodka, 2), (cranberry, 60), (lime_juice, 1), (triple_sec, 1)],
    #"Tequila Sunrise": [(tequila, 2), (grenadine, 15), (orange_juice, 90)],
    "Daiquiri": [(white_rum, 2), (lime_juice, 1), (simple_syrup, 1)],
    "Margarita": [(tequila, 2), (triple_sec, 1), (lime_juice, 1), (simple_syrup, 1)],
    "Long Island Iced Tea": [(vodka, 1), (dark_rum, 1), (tequila, 1), (gin, 1), (triple_sec, 1), (coke, 100)],
    "Rum & Coke": [(dark_rum, 2), (coke, 100)],
    "Cuba Libre": [(dark_rum, 2), (coke, 100), (lime_juice, 1)],
    "Gin Fizz": [(gin, 2), (lemon_juice, 1), (club_soda, 100), (simple_syrup, 1)],
    "Whiskey Sour": [(whiskey, 2), (lemon_juice, 1), (simple_syrup, 1)],
    "White Lady": [(gin, 2), (triple_sec, 1), (lemon_juice, 1)],
    "Vodka Cran": [(vodka, 2), (cranberry, 100)],
    #"Screwdriver": [(vodka, 2), (orange_juice, 90)],
    "Ginger Highball": [(whiskey, 2), (lemon_juice, 1), (simple_syrup, 1), (club_soda, 100)],
    "DO NOT ORDER" : [(coke, 50), (ginger_ale, 50), (orange_juice, 50), (club_soda, 50), (cranberry, 50)]
}

# do __name__.split('.')[0] if initialising from a file not at project root
app = flask.Flask(__name__, static_url_path='/static')
# app.debug = True
app.config['SERVER_NAME'] = '192.168.0.27:8080'  # Replace with your actual domain name
app.config['APPLICATION_ROOT'] = '/'  # Replace with your application root
app.config['PREFERRED_URL_SCHEME'] = 'http'  # Replace with your preferred URL scheme (http or https)

# Error handling and helpers
#
def error_info(e):
    content = ""
    try:  # it's probably a HttpException, if you're using the bigcommerce client
        content += str(e.headers) + "<br>" + str(e.content) + "<br>"
        req = e.response.request
        content += (
            "<br>Request:<br>"
            + req.url
            + "<br>"
            + str(req.headers)
            + "<br>"
            + str(req.body)
        )
    except AttributeError as e:  # not a HttpException
        content += "<br><br> (This page threw an exception: {})".format(str(e))
    return content


@app.errorhandler(500)
def internal_server_error(e):
    content = "Internal Server Error: " + str(e) + "<br>"
    content += error_info(e)
    return content, 500


@app.errorhandler(400)
def bad_request(e):
    content = "Bad Request: " + str(e) + "<br>"
    content += error_info(e)
    return content, 400


# Helper for template rendering
def render(template, context):
    return flask.render_template(template, **context)

#
# App interface
#
@app.route("/")
def index():
    # Render page
    context = dict()
    context["user"] = {
        "name": "John Doe",
        "email": "Justin",
    }
    context["menu"] = MENU
    return render("index.html", context)

def write_to_arduino(x):
    x = "FOO " + x
    arduino.write(bytes(x, 'utf-8'))
    arduino.flushInput()
    response = ""
    while "$ DONE" not in response:
        response += arduino.read().decode('utf-8')

    arduino.flushInput()
    print(x)

drink_making_in_progress = False

def make_drink(recipe_name):
    global drink_making_in_progress
    print("In make recipe")
    recipe = recipes[recipe_name]
    print(recipe)
    for item in recipe:
        write_to_arduino("MOVETO {}\n".format(item[0]))
        if item[0] < 100: 
            write_to_arduino("DISPENSE {} {}\n".format(item[0], item[1]))
        else: # pump
            write_to_arduino("PUMP {} {}\n".format(item[0] - 100, item[1]))

    write_to_arduino("HOME")
    time.sleep(1)
    drink_making_in_progress = False

drink_making_thread = None

@app.route("/make-drink", methods=["POST"])
def makeDrink():
    global drink_making_in_progress
    data = request.data  # data is empty
    # Render page
    print('Making drink', data)
    j = json.loads(data.decode('utf-8'))

    drink_making_thread = threading.Thread(target=make_drink, args=(j["drink"]["name"],))
    drink_making_in_progress = True
    drink_making_thread.start()
    print(j)
    print("-----------")
    print(j["drink"]["name"])

    return redirect(url_for('drink_in_progress'))

@app.route("/drink_in_progress")
def drink_in_progress():
    global drink_making_in_progress
    global drink_making_thread
    # This route will display the "drink in progress" page
    print("ARE WE MAKING A DRINK???")
    print(drink_making_in_progress)
    time.sleep(0.1)
    if drink_making_in_progress:
        return render_template("drink_in_progress.html", context={})
    else:
        if drink_making_thread is not None:
            drink_making_thread.join()
        return redirect(url_for('index'))

with app.app_context():
    MENU = {
        "config": {
            "confirmation1": "Place cup in the cup holder and confirm:",
            "confirmation2": "Confirm that you can see the emergency stop button (red button on your right)",
        },
        "drinks": [
            {
                "name": "DO NOT ORDER",
                "description": "DEV PURPOSES ONLY",
                "ingredients": ["60 whiskey", "30 lemon", "30 simple syrup"],
                "image": ""
            },
            {
                "name": "Lemon Drop",
                "description": "A sweet and tangy cocktail that combines the refreshing flavors of vodka, triple sec, lemon, and simple syrup. Served with a sugar rim, it's a delightful balance of citrus and sweetness.",
                "ingredients": ["60 vodka", "30 triple sec", "30 lemon", "30 simple syrup"],
                "image": url_for('static', filename='images/lemon_drop.jpg')
            },
            {
                "name": "Dark & Stormy",
                "description": "Transport yourself to the Caribbean with this classic dark rum cocktail. The Dark & Stormy blends the rich flavors of dark rum with the spiciness of ginger ale and a splash of lime. It's a perfect choice for those who enjoy a bold and flavorful drink.",
                "ingredients": ["60 dark rum", "ginger ale", "15 lime"],
                "image": url_for('static', filename='images/dark_stormy.jpg')
            },
            {
                "name": "Tom Collins",
                "description": "A timeless cocktail that exudes elegance and simplicity. The Tom Collins features gin, club soda, fresh lemon juice, and a touch of simple syrup. Crisp, refreshing, and garnished with a lemon twist, it's a classic choice for any occasion.",
                "ingredients": ["60 gin", "90 club soda", "30 simple syrup", "30 lemon juice"],
                "image": url_for('static', filename='images/tom_collins.jpg')
            },
            {
                "name": "Moscow Mule",
                "description": "Experience the zing of ginger and the smoothness of vodka in this popular cocktail. The Moscow Mule combines vodka, zesty lime juice, and ginger ale for a refreshing and invigorating drink. Served in a copper mug, it's a true crowd-pleaser.",
                "ingredients": ["60 vodka", "ginger ale", "15 lime"],
                "image": url_for('static', filename='images/moscow_mule.jpg')
            },
            {
                "name": "Cosmo",
                "description": "Step into the glamorous world of mixology with the Cosmopolitan. Made with vodka, cranberry juice, freshly squeezed lime juice, and a touch of triple sec, this cocktail is sophisticated and tangy. It's a perfect choice for those who appreciate a balance of flavors and a hint of citrus.",
                "ingredients": ["60 vodka", "30 cranberry juice", "15 lime juice", "15 triple sec"],
                "image": url_for('static', filename='images/cosmo.jpg')
            },
            {
                "name": "Daiquiri",
                "description": "Savor the simplicity of a Daiquiri. Made with white rum, freshly squeezed lime juice, and a touch of simple syrup, this cocktail is a timeless favorite. It offers a perfect balance of sweetness and tartness, making it a go-to choice for rum enthusiasts.",
                "ingredients": ["60 white rum", "30 lime", "30 simple syrup"],
                "image": url_for('static', filename='images/daiquiri.jpg')            
            },
            {
                "name": "Margarita",
                "description": "Get ready to enjoy the quintessential Mexican cocktail. The Margarita blends tequila, triple sec, freshly squeezed lime juice, and a touch of simple syrup for a refreshing and tangy experience. Served with a salt rim, it's a classic choice for those who appreciate the combination of tequila and citrus.",
                "ingredients": ["60 tequila", "30 triple sec", "30 lime", "30 simple syrup"],
                "image": url_for('static', filename='images/margarita.jpg')
            },
            {
                "name": "Long Island Iced Tea",
                "description": "Prepare for a potent mix of flavors in the Long Island Iced Tea. This cocktail combines vodka, dark rum, tequila, gin, triple sec, and a splash of cola to create a strong and invigorating drink. Despite its name, it does not actually contain tea but offers a unique blend of spirits.",
                "ingredients": ["30 vodka", "30 dark rum", "30 tequila", "30 gin", "30 triple sec", "cola"],
                "image": url_for('static', filename='images/lit.jpg')
            },
            {
                "name": "Rum & Coke",
                "description": "Sometimes simplicity is the key to perfection. The Rum & Coke, also known as the Cuba Libre, offers a delightful combination of dark rum and cola. Served with a squeeze of lime, it's a classic highball cocktail that delivers a smooth and refreshing experience.",
                "ingredients": ["60 dark rum", "cola"],
                "image": url_for('static', filename='images/rumncoke.jpg')
            },
            {
                "name": "Cuba Libre",
                "description": "Experience the vibrant and tropical flavors of Cuba with the Cuba Libre cocktail. Combining dark rum, cola, and a splash of lime juice, it's a refreshing drink that captures the essence of the Caribbean. Sip and imagine yourself on sandy beaches under swaying palm trees.",
                "ingredients": ["60 dark rum", "cola", "15 lime"],
                "image": url_for('static', filename='images/cuba.jpg')
            },
            {
                "name": "Gin Fizz",
                "description": "Effervescent and citrusy, the Gin Fizz is a classic cocktail that's perfect for gin enthusiasts. This refreshing drink combines gin, freshly squeezed lemon juice, club soda, and a touch of simple syrup. Served in a tall glass with ice, it's a delightful choice for hot summer days.",
                "ingredients": ["60 gin", "30 lemon", "90 club soda", "30 simple syrup"],
                "image": url_for('static', filename='images/gin_fizz.jpg')
            },
            {
                "name": "Whiskey Sour",
                "description": "Take pleasure in the timeless simplicity of a Whiskey Sour. This cocktail combines the rich flavors of whiskey, freshly squeezed lemon juice, and a touch of simple syrup. Garnished with a cherry, it's a classic choice for whiskey aficionados seeking a balanced blend of sweet and sour.",
                "ingredients": ["Whiskey", "30 lemon", "30 simple syrup"],
                "image": url_for('static', filename='images/whiskey_sour.jpg')
            },
            {
                "name": "White Lady",
                "description": "Elegant and sophisticated, the White Lady cocktail is a classic choice for gin lovers. Made with gin, triple sec, and freshly squeezed lemon juice, it offers a perfect balance of flavors. With a lemon twist garnish, it's a timeless drink that exudes refinement and style.",
                "ingredients": ["60 gin", "30 triple sec", "30 lemon"],
                "image": url_for('static', filename='images/white_lady.jpg')
            },
            {
                "name": "Vodka Cran",
                "description": "Embrace the bright and fruity flavors of a Vodka Cranberry cocktail. Combining vodka and cranberry juice, this refreshing drink offers a perfect blend of tartness and sweetness. Served over ice and garnished with a slice of lime, it's a popular choice for those who enjoy a vibrant and easy-to-sip cocktail.",
                "ingredients": ["60 vodka", "90 cranberry juice"],
                "image": url_for('static', filename='images/vodka_cran.jpg')
            },
            #{
            #    "name": "Screwdriver",
            #    "description": "Uncomplicated yet satisfying, the Screwdriver cocktail is a classic combination of vodka and orange juice. This simple drink is perfect for brunch or any casual gathering. Served over ice and garnished with an orange slice, it's a go-to choice for those seeking a refreshing and straightforward beverage.",
            #    "ingredients": ["60 vodka", "90 orange juice"],
            #    "image": url_for('static', filename='images/screwdriver.jpg')
            #},
            {
                "name": "Ginger Highball",
                "description": "The Ginger Highball is a delightful whiskey-based cocktail that offers a perfect balance of flavors. It combines whiskey, zesty lemon juice, and a touch of simple syrup. Topped with club soda, it delivers a refreshing and invigorating experience that whiskey enthusiasts will appreciate.",
                "ingredients": ["60 whiskey", "30 lemon", "30 simple syrup"],
                "image": url_for('static', filename='images/ginger_highball.jpg')
            }
        ]

    }

if __name__ == "__main__":
    app.run(host='0.0.0.0', port=8080)
