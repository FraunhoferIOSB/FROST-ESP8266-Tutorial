FROM nginx:1.17.1-alpine

# Replace with our custom nginx configuration
RUN rm /etc/nginx/conf.d/default.conf
COPY ./nginx.conf /etc/nginx/conf.d/

