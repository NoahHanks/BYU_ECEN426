import random


class Vars:
    UPPERCASE = 0x01
    LOWERCASE = 0x02
    REVERSE = 0x04
    SHUFFLE = 0x08
    RANDOM = 0x10


# Example of how to use it
# print(randomize_text(b"hello world")
# Method to randomize the input from the client
def randomize_text(text):
    def discard():
        return random.choices([True, False], weights=[1, 5])[0]

    def repeat(char):
        should_repeat = random.choices([True, False], weights=[1, 5])[0]
        if should_repeat:
            repeat_amount = int(random.paretovariate(1))
            return char * repeat_amount
        else:
            return char

    # text = text.decode()
    transformed_text = [repeat(c) for c in text if not discard()]
    if len(transformed_text) == 0:
        transformed_text = text[0]
    return "".join(transformed_text)  # .encode()
