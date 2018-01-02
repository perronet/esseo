# esseo
A metaphore of life.

## What this project is

This project explores the functionalities of the System V library via a simulation of a group of individuals. Since we are experimenting with IPC, signals and task scheduling, each individual is represented by a separate process.

### Quick start

In order to test the project, open the file called *sim_config* under */source* and tweak the parameters of the simulation. 
The following example contains the parameters you can tweak:

```
init_people=20
genes=500
sim_time=10
birth_death=1
```
 
Once this is done, you can open a terminal under */source* and use the following commands:

* *make run*: compile the project and run it.
* *make*: compile the project. Once this is done, you can use *./a.out* to run the simulation.
* *make test_session*: this is a handy command to use when you want to perform multiple tests automatically. This will performs 10 tests one after another. During a simulation, you can even tweak the parameters in *sim_config*. The updated parameters will be used in the next test.

### Testing the project

Inside the file *life_sim_lib.h* you can find some important globals of the projects :

* 

```
Give the example
```

And repeat

```
until finished
```

End with an example of getting some data out of the system or using it for a little demo

## Running the tests

Explain how to run the automated tests for this system

### Break down into end to end tests

Explain what these tests test and why

```
Give an example
```

### And coding style tests

Explain what these tests test and why

```
Give an example
```

## Deployment

Add additional notes about how to deploy this on a live system

## Built With

* [Dropwizard](http://www.dropwizard.io/1.0.2/docs/) - The web framework used
* [Maven](https://maven.apache.org/) - Dependency Management
* [ROME](https://rometools.github.io/rome/) - Used to generate RSS Feeds

## Contributing

Please read [CONTRIBUTING.md](https://gist.github.com/PurpleBooth/b24679402957c63ec426) for details on our code of conduct, and the process for submitting pull requests to us.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/your/project/tags). 

## Authors

* **Billie Thompson** - *Initial work* - [PurpleBooth](https://github.com/PurpleBooth)

See also the list of [contributors](https://github.com/your/project/contributors) who participated in this project.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Hat tip to anyone who's code was used
* Inspiration
* etc

