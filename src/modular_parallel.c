/* 
 * Search for pseudo collision in md5 hash funcion using a birthday attack approach
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<omp.h>

#include"md5/md5.h"
#include"generator/generator.h"

#define BUFFER_SIZE 22          /* The size for create a string representation of a number */
#define WARNS_AFTER 10000       /* Display a warning of status after X repetitions */

void parse_arguments(int argc, char *argv[], unsigned int *desired_collision, unsigned int *threads);       /* Function to parse arguments received from stdin */
void display_help_message();                        /* Display the parameters order and how to use properly */

int main(int argc, char *argv[])
{
    unsigned long long int *values;
    unsigned char **hashes;
    unsigned int desired_collision = 0;
    unsigned int threads = 0;

    parse_arguments(argc, argv, &desired_collision, &threads);
    char buffer[threads][BUFFER_SIZE];
    const unsigned __int128 iterations = calculate_iterations(desired_collision);

    values = malloc(sizeof(unsigned long long int) * iterations);
    hashes = malloc(sizeof(unsigned char*) * iterations);

    seed_generator();
    printf("==> Generating %lu random messages...\n", iterations);

    clock_t start = clock();

    # pragma omp parallel for num_threads(threads)
    for (int i = 0; i < iterations; i++){
        values[i] = generate_number(); 
        values[i] = (values[i] << 32) | generate_number();
    }

    printf("==> Hashing all %lu messages...\n", iterations);

    # pragma omp parallel for num_threads(threads)
    for (int i = 0; i < iterations; i++){
        unsigned char current_thread = omp_get_thread_num();

        snprintf(buffer[current_thread], BUFFER_SIZE, "%llu", values[i]);           /* Representing the number as string for hash process */
        hashes[i] = raw_md5(buffer[current_thread], strlen(buffer[current_thread]));      /* Get the hexadecimal md5 hash */
    }

    printf("==> Searching for collisions in hashes\n");

    for (int i = 0; i < iterations; i++){
        if (i % WARNS_AFTER == 0){
            printf("Already checked %d hashes\n", i); 
        }

        # pragma omp parallel for num_threads(threads)
        for (int j = i + 1; j < iterations; j++){
            unsigned char byte_collisions = 0;

            /* 
             * It's possible to have same numbers being generated by random function.
             * Of course, they are equal, not collision.
            */
            if (values[i] == values[j]){
                continue; 
            }

            /*
             * Desired collision is how many bytes (hexadecimal representation) must be equal
             * Each iteration of k, checks 2 hexadecimal bytes
             * As we do not have a way to have a half iteration hahahaha This case is treated inside the loop
             */
            for (int k = 0; k < (int)((desired_collision + 1) / 2); k++){
                /* 
                 * Storing temporarily the bytes to compare 
                 * I was having issue using bitshift operations in matrix implementation
                 */
                unsigned char first_byte = 0, second_byte = 0;

                first_byte = hashes[i][k];
                second_byte = hashes[j][k];

                /* Check the first 4 bits (hexadecimal byte) */
                if (((first_byte >> 4) ^ (second_byte >> 4)) == 0){
                    byte_collisions++;
                } 

                /* 
                 * If desired_collsion is odd, it must not check the next 4 bits
                 * Because it can create collisions that are not sequence
                 */
                if (k == ((int)((desired_collision) + 1) / 2) - 1){
                    break; 
                }

                /* Check the last 4 bits (hexadecimal byte) */
                if (((first_byte << 4) ^ (second_byte << 4)) == 0){
                    byte_collisions++;
                } 
            }

            if(byte_collisions == desired_collision){
                printf("==> %d bytes collision found!!!! Iteration: %d\n", byte_collisions, i);
                printf("md5('%llu')\t==\t%s\nmd5('%llu')\t==\t%s\n", values[i], get_hex_from_raw_digest(hashes[i]), values[j], get_hex_from_raw_digest(hashes[j]));
            }
        }
    }

    printf("Finished search for collision\n");
    printf("Time elapsed: %f seconds\n", ((double)clock() - start) / CLOCKS_PER_SEC);
    printf("Time elapsed: %f minutes\n", (((double)clock() - start) / CLOCKS_PER_SEC) / 60);
    printf("Time elapsed: %f hours\n", ((((double)clock() - start) / CLOCKS_PER_SEC) / 60) / 60);

    return 0;
}

void parse_arguments(int argc, char *argv[], unsigned int *desired_collision, unsigned int *threads)
{
    if (argc < 3){
        display_help_message(argv[0]); 
        exit(1);
    }

    *desired_collision = atoi(argv[1]);
    *threads = atoi(argv[2]);
}

void display_help_message(char *program_name)
{
    printf("Usage: %s [desired byte collision] [desired number of threads]\n", program_name);
    printf("[desired byte collision] = How many bytes must be equal for the hash be considered a collision\n");
    printf("[desired number of threads] = How many threads the program will generate to find a collision. Recomended: number of cores of your computer\n");
}