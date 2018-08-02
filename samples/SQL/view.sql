CREATE OR REPLACE VIEW v_email_template_company_standard AS
  SELECT
    c.id AS company_id,
    t1.use_id,
    COALESCE(t3.id, t2.id, t1.id) AS id,
    COALESCE(t3.type_id, t2.type_id, t1.type_id) AS type_id,
    COALESCE(t3.lang_id, t2.lang_id, t1.lang_id) AS lang_id,
    COALESCE(t3.subject, t2.subject, t1.subject) AS subject,
    COALESCE(t3.body, t2.body, t1.body) AS body
  FROM company c
  CROSS JOIN email_template t1
  LEFT JOIN email_template t2 ON t2.type_id = t1.type_id AND t2.use_id = t1.use_id AND t2.lang_id = c.lang_id
  LEFT JOIN (
    SELECT ct.company_id, t.id, t.type_id, t.lang_id, t.use_id, t.subject, t.body
    FROM company_email_template ct
    JOIN email_template t ON t.id = ct.template_id
  ) t3 ON t3.company_id = c.id AND t3.use_id = t1.use_id
  WHERE t1.type_id = (SELECT id FROM email_template_type WHERE title = 'system')
    AND t1.lang_id = (SELECT id FROM lang WHERE title = 'en');  


CREATE OR REPLACE VIEW v_fact AS
  SELECT
    f.id,
    f.company_id,
    f.is_trial,
    f.opened_on,
    f.closed_on,
    (f.opened_on <= now() AND now() < f.closed_on) AS is_active,
    (now() < f.opened_on) AS in_future,
    EXTRACT(EPOCH FROM (f.closed_on - f.opened_on))::int AS time_total,
    LEAST(EXTRACT(EPOCH FROM (now() - f.opened_on))::int, EXTRACT(EPOCH FROM (f.closed_on - f.opened_on))::int) AS time_used,
    GREATEST(EXTRACT(EPOCH FROM (f.closed_on - now()))::int, 0) AS time_left,
    (
      SELECT -sum(amount)
      FROM credit_transaction
      WHERE fact_id = f.id
    ) AS credits
  FROM fact f;  
  
  
  