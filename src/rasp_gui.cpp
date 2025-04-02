#include <QApplication> // Manages the Qt application and event loop
#include <QWidget>      // Base class for all UI windows and containers
#include <QLineEdit>    // Input box for entering LED color text
#include <QPushButton>  // Buttons for setting the LED and exiting the app
#include <QVBoxLayout>  // Arranges widgets vertically
#include <QLabel>       // Displays static text labels (e.g., window title)
#include <QFont>        // Controls font family, size, weight, etc.
#include <QPalette>     // Used to set GUI background color
#include <memory>       // Enables usage of std::unique_ptr for safety
#include <stdexcept>    // Allows throwing runtime errors during setup
#include <QString>      // Qt’s string class (used for typed input)
#include <pigpio.h>     // Library for Raspberry Pi GPIO access

// GPIO pin assignments for three LEDs
constexpr int RED_LED{17};   // GPIO 17 (physical pin 11)
constexpr int GREEN_LED{27}; // GPIO 27 (physical pin 13)
constexpr int BLUE_LED{22};  // GPIO 22 (physical pin 15)

/**
 * @brief Initializes the pigpio library and sets all LED pins as output.
 *        Throws an exception if pigpio fails to initialize (e.g., not running as root).
 */
void setupGpio()
{
    if (gpioInitialise() < 0)
    {
        throw std::runtime_error{"GPIO initialization failed"};
    }

    gpioSetMode(RED_LED, PI_OUTPUT);
    gpioSetMode(GREEN_LED, PI_OUTPUT);
    gpioSetMode(BLUE_LED, PI_OUTPUT);
}

/**
 * @brief Turns on only the specified LED and ensures the others are turned off.
 * @param pin The GPIO pin number for the LED to turn on.
 */
void turnOnOnly(int pin)
{
    gpioWrite(RED_LED, pin == RED_LED ? 1 : 0);
    gpioWrite(GREEN_LED, pin == GREEN_LED ? 1 : 0);
    gpioWrite(BLUE_LED, pin == BLUE_LED ? 1 : 0);
}

/**
 * @brief Creates the centered title label with custom font and white text.
 *        Returns a smart pointer, which will later be released to Qt's layout manager.
 */
std::unique_ptr<QLabel> createTitleLabel()
{
    auto label{std::make_unique<QLabel>("Text-Based LED Controller")};

    QFont font{"Arial", 16, QFont::Bold};
    label->setFont(font);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("QLabel { color: white; }");

    return label;
}

/**
 * @brief Creates the input section:
 *        - A QLineEdit for typing "red", "green", or "blue"
 *        - A QPushButton to apply the selection
 *
 * @return A QWidget containing both elements, laid out horizontally.
 */
std::unique_ptr<QWidget> createLedInputSection()
{
    // Input field where user types LED color
    auto input{std::make_unique<QLineEdit>()};
    input->setPlaceholderText("Type red / green / blue");
    input->setFont(QFont{"Arial", 12});
    input->setStyleSheet("QLineEdit { background-color: white; color: black; padding: 4px; }");

    // Button to trigger GPIO control
    auto button{std::make_unique<QPushButton>("Set LED")};
    button->setFont(QFont{"Arial", 12});
    button->setStyleSheet("QPushButton { background-color: grey; color: white; padding: 5px; }");

    // Connect button to the LED selection logic
    QObject::connect(button.get(), &QPushButton::clicked, [line = input.get()]()
                     {
        const QString text = line->text().trimmed().toLower();

        if (text == "red") {
            turnOnOnly(RED_LED);
        } else if (text == "green") {
            turnOnOnly(GREEN_LED);
        } else if (text == "blue") {
            turnOnOnly(BLUE_LED);
        } else {
            qWarning("Invalid color input: use red, green, or blue");
        } });

    // Place the input and button side-by-side
    auto layout{std::make_unique<QHBoxLayout>()};
    layout->addWidget(input.get());
    layout->addWidget(button.get());

    auto container{std::make_unique<QWidget>()};
    container->setLayout(layout.release());

    // When added to a layout, Qt assumes ownership.
    // We call release() to prevent double-deletion when smart pointers go out of scope.
    input.release();
    button.release();

    return container;
}

/**
 * @brief Creates the Exit button that closes the application.
 *        Uses smart pointer and later releases it to the layout.
 */
std::unique_ptr<QPushButton> createExitButton()
{
    auto button{std::make_unique<QPushButton>("Exit")};
    button->setFont(QFont{"Arial", 12});
    button->setStyleSheet("QPushButton { background-color: grey; color: white; padding: 5px; }");

    // Connect button to quit the application
    QObject::connect(button.get(), &QPushButton::clicked, []()
                     {
                         QApplication::quit(); // Triggers aboutToQuit for GPIO cleanup
                     });

    return button;
}

/**
 * @brief Creates the complete GUI window with title, input section, and Exit button.
 *        Returns a smart pointer that is safely managed in main().
 */
std::unique_ptr<QWidget> createGui()
{
    auto window{std::make_unique<QWidget>()};
    window->setWindowTitle("LED Control - Text Input");
    window->setFixedSize(420, 200);

    // Set dark theme background
    QPalette palette{window->palette()};
    palette.setColor(QPalette::Window, Qt::black);
    window->setAutoFillBackground(true);
    window->setPalette(palette);

    // Build subcomponents
    auto titleLabel{createTitleLabel()};
    auto inputSection{createLedInputSection()};
    auto exitButton{createExitButton()};
    auto layout{std::make_unique<QVBoxLayout>()};

    // Transfer widget ownership to Qt by releasing smart pointers
    layout->addWidget(titleLabel.release());
    layout->addWidget(inputSection.release());
    layout->addWidget(exitButton.get()); // Keep a reference to center align it
    layout->setAlignment(exitButton.get(), Qt::AlignCenter);
    exitButton.release(); // Now Qt owns the button

    layout->addStretch(); // Adds spacing at the bottom
    window->setLayout(layout.release());

    return window; // Return smart pointer holding the top-level window
}

/**
 * @brief Main function:
 *        - Initializes GPIO
 *        - Sets up automatic cleanup
 *        - Launches the Qt GUI
 */
int main(int argc, char *argv[])
{
    QApplication app{argc, argv};

    try
    {
        setupGpio(); // Initialize GPIO safely

        // Ensure GPIO cleanup on normal or forced app quit
        QObject::connect(&app, &QCoreApplication::aboutToQuit, []()
                         {
                             gpioWrite(RED_LED, 0);
                             gpioWrite(GREEN_LED, 0);
                             gpioWrite(BLUE_LED, 0);
                             gpioTerminate(); // Gracefully close pigpio
                         });

        // Show the GUI
        std::unique_ptr<QWidget> window{createGui()};
        window->show();

        return app.exec(); // Enter Qt event loop
    }
    catch (const std::exception &ex)
    {
        qCritical("Startup Error: %s", ex.what());
        gpioTerminate(); // Failsafe: release GPIO on failure
        return 1;
    }
}
