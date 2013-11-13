import re
from ert_gui.ide.keywords.definitions import ArgumentDefinition


class RangeStringArgument(ArgumentDefinition):

    NOT_A_VALID_RANGE_STRING = "The input should be of the type: <b><pre>\n\t1,3-5,9,17\n</pre></b>i.e. integer values separated by commas, and dashes to represent ranges."


    PATTERN = re.compile("^[0-9\-,]+$")
    RANGE_PATTERN = re.compile("^([0-9]+)-([0-9]+)$")
    NUMBER_PATTERN = re.compile("^([0-9]+)$")


    def __init__(self, **kwargs):
        super(RangeStringArgument, self).__init__(**kwargs)


    def validate(self, token):
        validation_status = super(RangeStringArgument, self).validate(token)

        if not validation_status:
            return validation_status
        else:
            match = RangeStringArgument.PATTERN.match(token)

            if match is None:
                validation_status.setFailed()
                validation_status.addToMessage(RangeStringArgument.NOT_A_VALID_RANGE_STRING)
            else:

                groups = token.split(",")

                for group in groups:
                    range_match = RangeStringArgument.RANGE_PATTERN.match(group)
                    number_match = RangeStringArgument.NUMBER_PATTERN.match(group)


                    if range_match is None and number_match is None:
                        validation_status.setFailed()
                        validation_status.addToMessage(RangeStringArgument.NOT_A_VALID_RANGE_STRING)
                        break

                    if range_match:
                        num_1 = int(range_match.group(1))
                        num_2 = int(range_match.group(2))

                        if not num_2 > num_1:
                            validation_status.setFailed()
                            validation_status.addToMessage(RangeStringArgument.NOT_A_VALID_RANGE_STRING)
                            break



                validation_status.setValue(token)

            return validation_status






