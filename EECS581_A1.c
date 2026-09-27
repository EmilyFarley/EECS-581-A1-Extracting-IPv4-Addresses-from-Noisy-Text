#include <stdio.h>
#include <string.h>

#define INPUT_SIZE 4096
#define TOKEN_SIZE 128

static int isDigit(char c)
{
    return c >= '0' && c <= '9';
}

static int isAddressChar(char c)
{
    return isDigit(c) || c == '.' || c == ':';
}

/* Parse an unsigned decimal field, enforcing its digit count and range. */
static int parseNumber(const char *text, size_t length,
                       size_t minDigits, size_t maxDigits,
                       unsigned int maximum, unsigned int *value)
{
    size_t i;
    unsigned int result = 0;

    if (length < minDigits || length > maxDigits)
        return 0;
    if (length > 1 && text[0] == '0')
        return 0;

    for (i = 0; i < length; ++i) {
        unsigned int digit;
        if (!isDigit(text[i]))
            return 0;
        digit = (unsigned int)(text[i] - '0');
        result = result * 10u + digit;
        if (result > maximum)
            return 0;
    }

    *value = result;
    return 1;
}

/* Validates one complete run made only of digits, periods, and colons. */
static int parseToken(const char *token, size_t length,
                      unsigned long *outAddress, int *outPort)
{
    unsigned int octets[4];
    unsigned int port = 0;
    size_t addressEnd = length;
    size_t colon = length;
    size_t start = 0;
    int octetIndex;
    unsigned long address;

    for (size_t i = 0; i < length; ++i) {
        if (token[i] == ':') {
            if (colon != length)
                return 0; /* A second colon is invalid. */
            colon = i;
        }
    }

    if (colon != length) {
        addressEnd = colon;
        if (!parseNumber(token + colon + 1, length - colon - 1,
                         1, 5, 65535u, &port))
            return 0;
    }

    for (octetIndex = 0; octetIndex < 4; ++octetIndex) {
        size_t end = start;
        while (end < addressEnd && token[end] != '.')
            ++end;

        if (!parseNumber(token + start, end - start, 1, 3, 255u,
                         &octets[octetIndex]))
            return 0;

        if (octetIndex < 3) {
            if (end >= addressEnd || token[end] != '.')
                return 0;
            start = end + 1;
        } else if (end != addressEnd) {
            /* Any extra period or component makes the whole token invalid. */
            return 0;
        }
    }

    address = ((unsigned long)octets[0] << 24)
            | ((unsigned long)octets[1] << 16)
            | ((unsigned long)octets[2] << 8)
            | (unsigned long)octets[3];

    *outAddress = address;
    *outPort = (colon == length) ? -1 : (int)port;
    return 1;
}

int extractIPv4(const char *str, unsigned long *outAddress, int *outPort)
{
    size_t i = 0;
    int found = 0;
    unsigned long foundAddress = 0;
    int foundPort = -1;

    if (outAddress == NULL || outPort == NULL)
        return 0;

    *outAddress = 0;
    *outPort = -1;
    if (str == NULL)
        return 0;

    while (str[i] != '\0') {
        size_t start;
        size_t length;
        unsigned long address;
        int port;

        while (str[i] != '\0' && !isAddressChar(str[i]))
            ++i;
        if (str[i] == '\0')
            break;

        start = i;
        while (str[i] != '\0' && isAddressChar(str[i]))
            ++i;
        length = i - start;

        if (!found && parseToken(str + start, length, &address, &port)) {
            found = 1;
            foundAddress = address;
            foundPort = port;
        }
    }

    if (!found)
        return 0;

    *outAddress = foundAddress;
    *outPort = foundPort;
    return 1;
}

int main(void)
{
    char input[INPUT_SIZE];

    for (;;) {
        unsigned long address;
        int port;
        unsigned int octets[4];

        printf("Enter a string (or 'END' to quit): ");
        if (fgets(input, sizeof input, stdin) == NULL)
            break;

        input[strcspn(input, "\n")] = '\0';
        if (strcmp(input, "END") == 0) {
            printf("Program terminated.\n");
            return 0;
        }

        if (extractIPv4(input, &address, &port)) {
            octets[0] = (unsigned int)((address >> 24) & 255ul);
            octets[1] = (unsigned int)((address >> 16) & 255ul);
            octets[2] = (unsigned int)((address >> 8) & 255ul);
            octets[3] = (unsigned int)(address & 255ul);

            printf("\nExtracted IPv4 address: %u.%u.%u.%u (decimal value: %lu, port: ",
                   octets[0], octets[1], octets[2], octets[3], address);
            if (port < 0)
                printf("none)\n\n");
            else
                printf("%d)\n\n", port);
        } else {
            printf("\nInvalid input: no valid IPv4 address found\n\n");
        }
    }

    printf("Program terminated.\n");
    return 0;
}
